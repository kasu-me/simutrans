/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_MCP_SERVER_H
#define NETWORK_MCP_SERVER_H

#include "network.h"
#include <string>
#include <vector>

class karte_t;
class script_vm_t;

/**
 * Minimal MCP (Model Context Protocol) server.
 * Listens on a TCP port, speaks JSON-RPC 2.0 over newline-delimited JSON.
 * step() must be called from the game main loop to process I/O on the main thread.
 */
class mcp_server_t
{
public:
	/**
	 * Start listening on the given port.
	 * @return true on success
	 */
	static bool init(uint16 port);

	/**
	 * Called once per game loop iteration (main thread).
	 * Accepts new connections, reads/writes data, dispatches requests.
	 * @param welt current world instance (may be NULL during startup)
	 */
	static void step(karte_t *welt);

	/** Close all connections and the listen socket. */
	static void shutdown();

	static bool is_active() { return listen_sock != INVALID_SOCKET  ||  listen_sock6 != INVALID_SOCKET; }

	/**
	 * While the main loop does not run (the game is saved, loaded, sent to a joining client):
	 * accepts connections and answers every request at once, tool calls with the error
	 * "busy: <reason>" (see live_changes_t::busy_t).
	 */
	static void step_busy(const char *reason);

	/**
	 * Requires the clients to authenticate: the token is the first line of the file.
	 * A connection must send it in initialize (params.auth_token) before anything else.
	 * @return false if the file cannot be read or the token is empty
	 */
	static bool read_token_file(const char *filename);

	// --- public JSON helpers (used by mcp_tools.cc) ---
	static std::string json_escape(const std::string &s);
	static std::string json_get_raw(const std::string &json, const std::string &key);
	static std::string json_get_string(const std::string &json, const std::string &key);

private:
	struct mcp_connection_t {
		SOCKET sock;
		std::string recv_buf;
		std::string send_buf;
		bool initialized; ///< true after "initialize" handshake completed
		bool authorized;  ///< true if no token is required or the client sent it
		explicit mcp_connection_t(SOCKET s) : sock(s), initialized(false), authorized(token.empty()) {}
	};

	/// A run_squirrel call that suspended mid-execution (network mode command_x).
	struct mcp_pending_t {
		script_vm_t      *vm;
		mcp_connection_t *conn;   ///< NULL when it was closed
		std::string       id_json;
		uint32            sync_start; ///< sync step when the script was called
		uint32            started_ms; ///< dr_time() when the script was called (for its time limit)
	};

	static SOCKET listen_sock;  ///< 127.0.0.1
	static SOCKET listen_sock6; ///< ::1
	static std::vector<mcp_connection_t *> connections;
	static std::vector<mcp_pending_t *>    pending_scripts;

	/// A save_world call: done after the requests of a step (see step_saves).
	struct mcp_pending_save_t {
		mcp_connection_t *conn;   ///< NULL when it was closed
		std::string       id_json;
		std::string       args_json;
	};
	static std::vector<mcp_pending_save_t> pending_saves;
	static karte_t *world; ///< current world, set each step()
	static std::string token; ///< required in initialize if not empty
	static const char *busy; ///< reason while step_busy() handles the requests
	static bool stepping;    ///< in step(): requests are being handled

	/** Reads the connections and handles the complete lines, then writes what is pending. */
	static void io_connections();

	static SOCKET open_listener(int family, uint16 port);
	static void accept_new(SOCKET listener);
	static void io_connection(mcp_connection_t *c);
	static void handle_line(mcp_connection_t *c, const std::string &line);
	static void step_pending();
	static void step_saves();

	/** Build a JSON-RPC success response. result_json is the raw JSON value. */
	static std::string make_response(const std::string &id_json, const std::string &result_json);

	/** Build a JSON-RPC error response. */
	static std::string make_error(const std::string &id_json, int code, const std::string &message);

	/**
	 * Dispatch a parsed request.
	 * @return JSON-RPC response string, or "" if the response will be sent asynchronously.
	 */
	static std::string dispatch(mcp_connection_t   *c,
	                            const std::string  &method,
	                            const std::string  &id_json,
	                            const std::string  &params_json);

	static void close_socket(SOCKET s);

	/** Reads 4 hex digits at pos (for \uXXXX). */
	static bool parse_hex4(const std::string &s, size_t pos, uint32 &value);

	static void append_utf8(std::string &out, uint32 cp);
};

#endif // NETWORK_MCP_SERVER_H
