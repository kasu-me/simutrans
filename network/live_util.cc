/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "live_util.h"
#include "mcp_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <charconv>

#include "../descriptor/vehicle_desc.h"


void live_util::add_int(std::string &out, sint64 v)
{
	char buf[24];
	const std::to_chars_result r = std::to_chars(buf, buf + sizeof(buf), v);
	out.append(buf, r.ptr - buf);
}


void live_util::add_double(std::string &out, double v, int decimals)
{
	char buf[40];
	snprintf(buf, sizeof(buf), "%.*f", decimals, v);
	out += buf;
}


void live_util::add_str(std::string &out, const char *s)
{
	out += '"';
	out += mcp_server_t::json_escape(s ? s : "");
	out += '"';
}


std::string live_util::error_json(const char *msg)
{
	std::string out = "{\"error\":";
	add_str(out, msg);
	out += '}';
	return out;
}


bool live_util::get_int_arg(const std::string &args_json, const char *key, sint64 &value)
{
	const std::string raw = mcp_server_t::json_get_raw(args_json, key);
	if(  raw.empty()  ||  raw == "null"  ) {
		return false;
	}
	char *end;
	value = strtoll(raw.c_str(), &end, 10);
	return end != raw.c_str()  &&  *end == 0;
}


bool live_util::get_bool_arg(const std::string &args_json, const char *key)
{
	return mcp_server_t::json_get_raw(args_json, key) == "true";
}


uint32 live_util::fnv1a(const char *data, size_t len, uint32 h)
{
	for(  size_t i = 0;  i < len;  i++  ) {
		h = (h ^ (uint8)data[i]) * 16777619u;
	}
	return h;
}


// ---------------------------------------------------------------------------
// vehicle names
// ---------------------------------------------------------------------------

namespace {
std::unordered_map<const vehicle_desc_t *, uint32> vehicle_name_indices;
std::vector<const vehicle_desc_t *> vehicle_names;
}


uint32 live_util::vehicle_name_index(const vehicle_desc_t *desc)
{
	auto it = vehicle_name_indices.find(desc);
	if(  it != vehicle_name_indices.end()  ) {
		return it->second;
	}
	const uint32 idx = (uint32)vehicle_names.size();
	vehicle_names.push_back(desc);
	vehicle_name_indices[desc] = idx;
	return idx;
}


void live_util::add_vehicle_names(std::string &out)
{
	out += "\"vnames\":[";
	for(  size_t i = 0;  i < vehicle_names.size();  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		add_str(out, vehicle_names[i]->get_name());
	}
	out += ']';
}


// ---------------------------------------------------------------------------
// change detection
// ---------------------------------------------------------------------------

uint32 live_util::entity_changes_t::seq = 0;

/// more forgotten entities than this are dropped (answers for an older `since` become full)
static const size_t MAX_TOMBSTONES = 4096;


void live_util::entity_changes_t::begin(bool has_since, uint32 since_)
{
	calls++;
	since = since_;
	// the first call cannot know what disappeared before
	full = !has_since  ||  calls == 1  ||  since < floor  ||  since > seq;
}


bool live_util::entity_changes_t::update(uint64 key, const std::string &record)
{
	const uint32 h = fnv1a(record);
	auto it = entries.find(key);
	if(  it == entries.end()  ) {
		it = entries.emplace(key, entry_t{ h, ++seq, calls, false }).first;
	}
	else {
		entry_t &e = it->second;
		if(  e.gone  ||  e.hash != h  ) {
			e.hash = h;
			e.seq = ++seq;
			e.gone = false;
		}
		e.seen = calls;
	}
	return full  ||  it->second.seq > since;
}


std::vector<uint64> live_util::entity_changes_t::end()
{
	std::vector<uint64> gone;
	size_t tombstones = 0;
	for(  auto &kv : entries  ) {
		entry_t &e = kv.second;
		if(  !e.gone  &&  e.seen != calls  ) {
			e.gone = true;
			e.seq = ++seq;
		}
		if(  e.gone  ) {
			tombstones++;
			if(  !full  &&  e.seq > since  ) {
				gone.push_back(kv.first);
			}
		}
	}
	if(  tombstones > MAX_TOMBSTONES  ) {
		for(  auto it = entries.begin();  it != entries.end();  ) {
			if(  it->second.gone  ) {
				it = entries.erase(it);
			}
			else {
				++it;
			}
		}
		floor = seq;
	}
	return gone;
}


void live_util::entity_changes_t::clear()
{
	entries.clear();
	calls = 0;
	floor = 0;
}


namespace {
std::unordered_map<std::string, live_util::entity_changes_t> all_changes;
}


live_util::entity_changes_t &live_util::changes_of(const std::string &kind)
{
	if(  all_changes.size() > 32  &&  all_changes.find(kind) == all_changes.end()  ) {
		// too many shapes of requests: start over (the callers get full answers)
		all_changes.clear();
	}
	return all_changes[kind];
}


void live_util::clear_all_changes()
{
	for(  auto &kv : all_changes  ) {
		kv.second.clear();
	}
}
