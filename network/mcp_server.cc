/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * Minimal MCP (Model Context Protocol) server implementation.
 *
 * Transport: raw TCP, newline-delimited JSON (NDJSON).
 * Each JSON-RPC 2.0 message is one line terminated by LF.
 *
 * Supported methods:
 *   initialize          → reply with server capabilities
 *   notifications/...   → no reply (notifications from client)
 *   tools/list          → return registered tools (see mcp_tools.cc)
 *   tools/call          → execute a tool (see mcp_tools.cc)
 *   ping                → return empty result
 */

#include "mcp_server.h"

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#if !USE_WINSOCK
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "../simdebug.h"
#include "../simversion.h"
#include "../simworld.h"
#include "../script/script.h"
#include "../utils/plainstring.h"
#include "mcp_tools.h"
#include "live_state.h"

// ---- static member storage ----
SOCKET mcp_server_t::listen_sock = INVALID_SOCKET;
SOCKET mcp_server_t::listen_sock6 = INVALID_SOCKET;
std::vector<mcp_server_t::mcp_connection_t *> mcp_server_t::connections;
std::vector<mcp_server_t::mcp_pending_t *>    mcp_server_t::pending_scripts;
karte_t *mcp_server_t::world = NULL;
std::string mcp_server_t::token;
const char *mcp_server_t::busy = NULL;
bool mcp_server_t::stepping = false;


// ---- platform helpers ----

static void mcp_set_nonblocking(SOCKET s)
{
#if USE_WINSOCK
	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);
#else
	int flags = fcntl(s, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(s, F_SETFL, flags | O_NONBLOCK);
	}
#endif
}

void mcp_server_t::close_socket(SOCKET s)
{
	if (s != INVALID_SOCKET) {
#if USE_WINSOCK
		closesocket(s);
#else
		close(s);
#endif
	}
}


// ---- public API ----

bool mcp_server_t::init(uint16 port)
{
	if (is_active()) {
		dbg->warning("mcp_server_t::init", "already running");
		return false;
	}

#if USE_WINSOCK
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		dbg->warning("mcp_server_t::init", "WSAStartup failed");
		return false;
	}
#endif

	// One socket per loopback address: a dual-stack socket bound to ::1 does not accept 127.0.0.1
	listen_sock  = open_listener(AF_INET, port);
	listen_sock6 = open_listener(AF_INET6, port);
	if (!is_active()) {
		return false;
	}

	live_changes_t::set_enabled(true);
	dbg->message("mcp_server_t::init", "MCP server listening on port %u (%s%s%s)", port,
		listen_sock != INVALID_SOCKET ? "127.0.0.1" : "",
		listen_sock != INVALID_SOCKET  &&  listen_sock6 != INVALID_SOCKET ? ", " : "",
		listen_sock6 != INVALID_SOCKET ? "::1" : "");
	return true;
}


SOCKET mcp_server_t::open_listener(int family, uint16 port)
{
	const char *addr_name = family == AF_INET6 ? "::1" : "127.0.0.1";
	SOCKET s = socket(family, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		dbg->warning("mcp_server_t::init", "socket() failed for %s", addr_name);
		return INVALID_SOCKET;
	}

	int on = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

	int err;
	if (family == AF_INET6) {
		// the IPv4 loopback has its own socket
		setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&on, sizeof(on));

		struct sockaddr_in6 addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin6_family = AF_INET6;
		addr.sin6_port   = htons(port);
		addr.sin6_addr   = in6addr_loopback;
		err = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	}
	else {
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family      = AF_INET;
		addr.sin_port        = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		err = bind(s, (struct sockaddr *)&addr, sizeof(addr));
	}
	if (err != 0) {
		dbg->warning("mcp_server_t::init", "bind() failed on %s port %u (error %d)", addr_name, port, (int)GET_LAST_ERROR());
		close_socket(s);
		return INVALID_SOCKET;
	}

	if (listen(s, 8) != 0) {
		dbg->warning("mcp_server_t::init", "listen() failed on %s (error %d)", addr_name, (int)GET_LAST_ERROR());
		close_socket(s);
		return INVALID_SOCKET;
	}

	mcp_set_nonblocking(s);
	return s;
}


void mcp_server_t::shutdown()
{
	for (mcp_pending_t *p : pending_scripts) {
		delete p->vm;
		delete p;
	}
	pending_scripts.clear();

	for (mcp_connection_t *c : connections) {
		close_socket(c->sock);
		delete c;
	}
	connections.clear();
	close_socket(listen_sock);
	listen_sock = INVALID_SOCKET;
	close_socket(listen_sock6);
	listen_sock6 = INVALID_SOCKET;
	live_changes_t::set_enabled(false);
}


void mcp_server_t::step(karte_t *welt)
{
	world = welt;
	if (!is_active()) {
		return;
	}

	stepping = true;
	io_connections();
	step_pending();
	stepping = false;
}


void mcp_server_t::step_busy(const char *reason)
{
	// not from within a request (a tool that saves or loads)
	if (!is_active()  ||  stepping) {
		return;
	}
	busy = reason;
	io_connections();
	busy = NULL;
}


bool mcp_server_t::read_token_file(const char *filename)
{
	FILE *f = fopen(filename, "rb");
	if (!f) {
		return false;
	}
	char buf[256];
	const bool ok = fgets(buf, sizeof(buf), f) != NULL;
	fclose(f);
	if (!ok) {
		return false;
	}
	token = buf;
	while (!token.empty()  &&  (token.back() == '\n'  ||  token.back() == '\r'  ||  token.back() == ' ')) {
		token.pop_back();
	}
	return !token.empty();
}


void mcp_server_t::io_connections()
{
	accept_new(listen_sock);
	accept_new(listen_sock6);

	// Process existing connections; collect dead ones
	std::vector<mcp_connection_t *> dead;
	for (mcp_connection_t *c : connections) {
		io_connection(c);
		if (c->sock == INVALID_SOCKET) {
			dead.push_back(c);
		}
	}
	for (mcp_connection_t *c : dead) {
		connections.erase(std::find(connections.begin(), connections.end(), c));
		delete c;
	}
}


// ---- private: pending script resume ----

void mcp_server_t::step_pending()
{
	if (pending_scripts.empty()) {
		return;
	}

	std::vector<mcp_pending_t *> still_pending;
	for (mcp_pending_t *p : pending_scripts) {
		// If the connection was closed, cancel the script silently
		if (p->conn->sock == INVALID_SOCKET) {
			delete p->vm;
			delete p;
			continue;
		}

		plainstring result;
		const char *err = p->vm->try_continue(result);

		// when the call was made, and when the server executed the commands (the script resumes after them)
		char syncs[64];
		snprintf(syncs, sizeof(syncs), ",\"sync_start\":%u,\"sync\":%u}", p->sync_start, world ? world->get_sync_steps() : 0);
		std::string result_json;
		if (err == NULL) {
			// Completed successfully
			result_json = std::string("{\"result\":\"")
			            + json_escape(result.c_str() ? result.c_str() : "") + "\"" + mcp_tools::script_ops(p->vm) + syncs;
		}
		else if (strcmp(err, "suspended") == 0) {
			// Still waiting for network command, or more opcodes to run: unless the script ran out of its limits
			const char *limit = mcp_tools::script_limit(p->vm, p->started_ms);
			if (!limit) {
				still_pending.push_back(p);
				continue;
			}
			result_json = std::string("{\"error\":\"") + json_escape(limit) + "\"" + mcp_tools::script_ops(p->vm) + syncs;
		}
		else {
			// Error
			result_json = std::string("{\"error\":\"") + json_escape(mcp_tools::script_error(p->vm, err)) + "\"" + mcp_tools::script_ops(p->vm) + syncs;
		}

		// Wrap and send response
		std::string escaped = json_escape(result_json);
		std::string content = "{\"content\":[{\"type\":\"text\",\"text\":\""
		                    + escaped + "\"}]}";
		p->conn->send_buf += make_response(p->id_json, content) + '\n';
		delete p->vm;
		delete p;
	}
	pending_scripts = std::move(still_pending);
}


// ---- private: network I/O ----

void mcp_server_t::accept_new(SOCKET listener)
{
	if (listener == INVALID_SOCKET) {
		return;
	}
	for (;;) {
		struct sockaddr_storage addr;
		socklen_t addrlen = sizeof(addr);
		SOCKET client = accept(listener, (struct sockaddr *)&addr, &addrlen);
		if (client == INVALID_SOCKET) {
			break; // no more pending connections
		}
		mcp_set_nonblocking(client);
		// TCP_NODELAY for low latency
		{
			int on = 1;
			setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(on));
		}
		connections.push_back(new mcp_connection_t(client));
		dbg->message("mcp_server_t::accept_new", "MCP client connected");
	}
}


void mcp_server_t::io_connection(mcp_connection_t *c)
{
	// --- read ---
	bool peer_closed = false;
	char buf[4096];
	for (;;) {
#if USE_WINSOCK
		int n = recv(c->sock, buf, sizeof(buf), 0);
#else
		ssize_t n = recv(c->sock, buf, sizeof(buf), 0);
#endif
		if (n > 0) {
			c->recv_buf.append(buf, n);
		}
		else if (n == 0) {
			// Half-close: peer finished sending. Process buffered data then close after flush.
			peer_closed = true;
			break;
		}
		else {
			break; // EWOULDBLOCK or transient error; stop reading for now
		}
	}

	// Guard: if recv_buf is unreasonably large (broken client), disconnect
	if (c->recv_buf.size() > 1024 * 1024) {
		dbg->warning("mcp_server_t::io_connection", "recv_buf overflow, closing client");
		close_socket(c->sock);
		c->sock = INVALID_SOCKET;
		return;
	}

	// Process complete lines
	for (;;) {
		size_t nl = c->recv_buf.find('\n');
		if (nl == std::string::npos) {
			break;
		}
		std::string line = c->recv_buf.substr(0, nl);
		c->recv_buf.erase(0, nl + 1);
		// strip trailing \r if present
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (!line.empty()) {
			handle_line(c, line);
		}
	}

	// --- write pending send_buf ---
	while (c->sock != INVALID_SOCKET && !c->send_buf.empty()) {
#if USE_WINSOCK
		int n = send(c->sock, c->send_buf.data(), (int)c->send_buf.size(), 0);
#else
		ssize_t n = send(c->sock, c->send_buf.data(), c->send_buf.size(), 0);
#endif
		if (n > 0) {
			c->send_buf.erase(0, n);
		}
		else {
			break; // EWOULDBLOCK or error
		}
	}

	// Close after all pending data flushed, if peer already closed
	if (peer_closed && c->send_buf.empty()) {
		dbg->message("mcp_server_t::io_connection", "MCP client disconnected");
		close_socket(c->sock);
		c->sock = INVALID_SOCKET;
	}
}


void mcp_server_t::handle_line(mcp_connection_t *c, const std::string &line)
{
	std::string method  = json_get_string(line, "method");
	std::string id_json = json_get_raw(line, "id");
	std::string params  = json_get_raw(line, "params");

	if (method.empty()) {
		dbg->warning("mcp_server_t::handle_line", "no method in JSON: %.200s", line.c_str());
		return;
	}

	if (!c->authorized) {
		if (method == "initialize"  &&  json_get_string(params, "auth_token") == token) {
			c->authorized = true;
		}
		else {
			if (!id_json.empty()  &&  id_json != "null") {
				c->send_buf += make_error(id_json, -32001, "unauthorized: send the token as params.auth_token of initialize") + '\n';
			}
			return;
		}
	}

	std::string response = dispatch(c, method, id_json, params);
	if (!response.empty()) {
		c->send_buf += response;
		c->send_buf += '\n';
	}
	// If response is empty and this was a tools/call, the script may have suspended;
	// the pending VM was already added to pending_scripts inside dispatch().
}


// ---- private: dispatch ----

std::string mcp_server_t::dispatch(mcp_connection_t   *c,
                                    const std::string  &method,
                                    const std::string  &id_json,
                                    const std::string  &params_json)
{
	// Notifications (no id) → no response
	if (id_json.empty() || id_json == "null") {
		dbg->message("mcp_server_t::dispatch", "notification: %s", method.c_str());
		return "";
	}

	// --- initialize ---
	if (method == "initialize") {
		const char *result =
			"{"
			"\"protocolVersion\":\"2024-11-05\","
			"\"serverInfo\":{"
				"\"name\":\"simutrans-otrp\","
				"\"version\":\"" VERSION_NUMBER "\""
			"},"
			"\"capabilities\":{"
				"\"tools\":{}"
			"}"
			"}";
		return make_response(id_json, result);
	}

	// --- ping ---
	if (method == "ping") {
		return make_response(id_json, "{}");
	}

	// --- tools/list ---
	if (method == "tools/list") {
		std::string body = "{\"tools\":" + mcp_tools::tools_list_json() + "}";
		return make_response(id_json, body);
	}

	// --- tools/call ---
	if (method == "tools/call") {
		std::string tool_name = json_get_string(params_json, "name");
		std::string args_json = json_get_raw(params_json, "arguments");
		if (args_json.empty()) args_json = "{}";

		if (busy) {
			// the game is not in a state to be read or changed: answer at once instead of after the save
			return make_response(id_json, mcp_tools::busy_result(busy));
		}

		script_vm_t *pending_vm = nullptr;
		std::string result_json = mcp_tools::tools_call(tool_name, args_json, world,
		                                                 &pending_vm);
		if (pending_vm) {
			// Script suspended (network mode command_x) — response will be sent later
			pending_scripts.push_back(new mcp_pending_t{pending_vm, c, id_json, world ? world->get_sync_steps() : 0, mcp_tools::script_started_ms()});
			return "";
		}
		return make_response(id_json, result_json);
	}

	// --- unknown method ---
	return make_error(id_json, -32601, "method not found: " + method);
}


// ---- private: JSON helpers ----

std::string mcp_server_t::make_response(const std::string &id_json, const std::string &result_json)
{
	return std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_json + ",\"result\":" + result_json + "}";
}

std::string mcp_server_t::make_error(const std::string &id_json, int code, const std::string &message)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d", code);
	return std::string("{\"jsonrpc\":\"2.0\",\"id\":") + id_json +
	       ",\"error\":{\"code\":" + buf + ",\"message\":\"" + json_escape(message) + "\"}}";
}

std::string mcp_server_t::json_escape(const std::string &s)
{
	std::string out;
	out.reserve(s.size());
	for (unsigned char c : s) {
		switch (c) {
			case '"':  out += "\\\""; break;
			case '\\': out += "\\\\"; break;
			case '\n': out += "\\n";  break;
			case '\r': out += "\\r";  break;
			case '\t': out += "\\t";  break;
			default:
				if (c < 0x20) {
					char hex[8];
					snprintf(hex, sizeof(hex), "\\u%04x", c);
					out += hex;
				} else {
					out += (char)c;
				}
		}
	}
	return out;
}

/**
 * Find the raw JSON value for the first occurrence of "key": in a JSON object.
 * Returns the raw JSON token/value (object, array, string, number, bool, null).
 * Handles one level of nesting for objects and arrays.
 * Not a full JSON parser – sufficient for well-formed JSON-RPC envelopes.
 */
std::string mcp_server_t::json_get_raw(const std::string &json, const std::string &key)
{
	// Search for  "key":
	std::string needle = "\"" + key + "\"";
	size_t pos = json.find(needle);
	if (pos == std::string::npos) {
		return "";
	}
	pos += needle.size();
	// skip whitespace and ':'
	while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) {
		++pos;
	}
	if (pos >= json.size() || json[pos] != ':') {
		return "";
	}
	++pos;
	while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
		++pos;
	}
	if (pos >= json.size()) {
		return "";
	}

	char first = json[pos];
	if (first == '{' || first == '[') {
		// Scan to matching bracket
		char open  = first;
		char close = (first == '{') ? '}' : ']';
		int depth = 0;
		size_t start = pos;
		bool in_str = false;
		for (; pos < json.size(); ++pos) {
			char c = json[pos];
			if (in_str) {
				if (c == '\\') { ++pos; continue; }
				if (c == '"')  { in_str = false; }
			} else {
				if (c == '"')  { in_str = true; }
				else if (c == open)  { ++depth; }
				else if (c == close) { --depth; if (depth == 0) { ++pos; break; } }
			}
		}
		return json.substr(start, pos - start);
	}
	else if (first == '"') {
		// String: find closing quote respecting escapes
		size_t start = pos;
		++pos;
		bool escaped = false;
		for (; pos < json.size(); ++pos) {
			if (escaped)      { escaped = false; continue; }
			if (json[pos] == '\\') { escaped = true; continue; }
			if (json[pos] == '"')  { ++pos; break; }
		}
		return json.substr(start, pos - start);
	}
	else {
		// Number, bool, null: scan until delimiter
		size_t start = pos;
		while (pos < json.size()) {
			char c = json[pos];
			if (c == ',' || c == '}' || c == ']' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				break;
			}
			++pos;
		}
		return json.substr(start, pos - start);
	}
}

std::string mcp_server_t::json_get_string(const std::string &json, const std::string &key)
{
	std::string raw = json_get_raw(json, key);
	if (raw.size() < 2 || raw.front() != '"' || raw.back() != '"') {
		return raw; // not a JSON string literal, return as-is
	}
	// Unescape basic sequences
	std::string out;
	out.reserve(raw.size());
	for (size_t i = 1; i + 1 < raw.size(); ++i) {
		if (raw[i] == '\\' && i + 2 < raw.size()) {
			++i;
			switch (raw[i]) {
				case '"':  out += '"';  break;
				case '\\': out += '\\'; break;
				case '/':  out += '/';  break;
				case 'n':  out += '\n'; break;
				case 'r':  out += '\r'; break;
				case 't':  out += '\t'; break;
				case 'b':  out += '\b'; break;
				case 'f':  out += '\f'; break;
				case 'u': {
					// \uXXXX (clients like Python's json escape all non-ASCII text this way), surrogate pairs included
					uint32 cp;
					if (!parse_hex4(raw, i + 1, cp)) {
						out += raw[i];
						break;
					}
					i += 4;
					uint32 low;
					if (cp >= 0xD800 && cp < 0xDC00 && i + 6 < raw.size() && raw[i + 1] == '\\' && raw[i + 2] == 'u'
					    && parse_hex4(raw, i + 3, low) && low >= 0xDC00 && low < 0xE000) {
						cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
						i += 6;
					}
					append_utf8(out, cp);
					break;
				}
				default:   out += raw[i]; break;
			}
		} else {
			out += raw[i];
		}
	}
	return out;
}


bool mcp_server_t::parse_hex4(const std::string &s, size_t pos, uint32 &value)
{
	if (pos + 4 > s.size()) {
		return false;
	}
	value = 0;
	for (size_t k = pos; k < pos + 4; ++k) {
		const char c = s[k];
		value <<= 4;
		if (c >= '0' && c <= '9')      value |= c - '0';
		else if (c >= 'a' && c <= 'f') value |= c - 'a' + 10;
		else if (c >= 'A' && c <= 'F') value |= c - 'A' + 10;
		else return false;
	}
	return true;
}


void mcp_server_t::append_utf8(std::string &out, uint32 cp)
{
	if (cp < 0x80) {
		out += (char)cp;
	}
	else if (cp < 0x800) {
		out += (char)(0xC0 | (cp >> 6));
		out += (char)(0x80 | (cp & 0x3F));
	}
	else if (cp < 0x10000) {
		out += (char)(0xE0 | (cp >> 12));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	}
	else {
		out += (char)(0xF0 | (cp >> 18));
		out += (char)(0x80 | ((cp >> 12) & 0x3F));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	}
}
