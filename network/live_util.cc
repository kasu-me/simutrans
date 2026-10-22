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
#include "../dataobj/schedule.h"
#include "../simline.h"


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


std::vector<std::string> live_util::parse_string_array(const std::string &raw)
{
	std::vector<std::string> list;
	size_t pos = 0;
	while(  (pos = raw.find('"', pos)) != std::string::npos  ) {
		const size_t end = raw.find('"', pos + 1);
		if(  end == std::string::npos  ) {
			break;
		}
		list.push_back(raw.substr(pos + 1, end - pos - 1));
		pos = end + 1;
	}
	return list;
}


std::vector<sint64> live_util::parse_int_array(const std::string &raw)
{
	std::vector<sint64> list;
	const char *p = raw.c_str();
	while(  *p  ) {
		if(  (*p >= '0'  &&  *p <= '9')  ||  *p == '-'  ) {
			char *end;
			list.push_back(strtoll(p, &end, 10));
			p = end;
		}
		else {
			p++;
		}
	}
	return list;
}


std::vector<std::vector<sint64>> live_util::parse_int_groups(const std::string &raw)
{
	std::vector<std::vector<sint64>> groups;
	int depth = 0;
	const char *p = raw.c_str();
	while(  *p  ) {
		if(  *p == '['  ) {
			depth++;
			if(  depth == 2  ) {
				groups.emplace_back();
			}
			p++;
		}
		else if(  *p == ']'  ) {
			depth--;
			p++;
		}
		else if(  (*p >= '0'  &&  *p <= '9')  ||  *p == '-'  ) {
			char *end;
			const sint64 v = strtoll(p, &end, 10);
			if(  depth <= 1  ) {
				groups.emplace_back(1, v);
			}
			else {
				groups.back().push_back(v);
			}
			p = end;
		}
		else {
			p++;
		}
	}
	return groups;
}


namespace {

/// FNV-1a over the little-endian bytes of v (width bytes)
uint32 hash_le(uint32 h, uint64 v, int width)
{
	for(  int i = 0;  i < width;  i++  ) {
		h = (h ^ (uint8)(v >> (8 * i))) * 16777619u;
	}
	return h;
}

}


uint32 live_util::schedule_hash(const schedule_t *schedule)
{
	if(  !schedule  ) {
		return 0;
	}
	uint32 h = 2166136261u;
	h = hash_le(h, schedule->get_flags(), 1);
	h = hash_le(h, schedule->get_max_speed(), 2);
	h = hash_le(h, schedule->get_departure_slot_group_id().get_id(), 4);
	h = hash_le(h, schedule->get_additional_base_waiting_time(), 4);
	h = hash_le(h, (uint8)schedule->get_waytype(), 1);
	h = hash_le(h, schedule->get_next_line().get_id(), 4);
	h = hash_le(h, schedule->get_count(), 1);
	for(  const schedule_entry_t &e : schedule->get_entries()  ) {
		h = hash_le(h, (uint16)e.pos.x, 2);
		h = hash_le(h, (uint16)e.pos.y, 2);
		h = hash_le(h, (uint8)e.pos.z, 1);
		h = hash_le(h, e.minimum_loading, 1);
		h = hash_le(h, e.waiting_time_shift, 2);
		h = hash_le(h, e.get_stop_flags(), 4);
		h = hash_le(h, e.max_speed_kmh_of_convoi, 2);
		h = hash_le(h, e.spacing, 2);
		h = hash_le(h, e.spacing_shift, 2);
		h = hash_le(h, e.delay_tolerance, 2);
		h = hash_le(h, e.length_coupling_done, 2);
		h = hash_le(h, e.maximum_loading, 1);
		h = hash_le(h, e.balance_speed_kmh_of_convoi, 2);
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
