/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * Read-only MCP tools for a live view of the game (a web viewer bridge).
 *
 * get_live_state  every convoy and vehicle as it is now (no state is kept
 *                 between calls: the caller computes differences)
 * get_tiles       a rectangle of tiles in the savegame format
 * get_changes     the tiles and signal states changed since a sequence number
 * dump_vehicles   all fields of some convoys, for diagnosis
 *
 * All of them run on the main thread (from mcp_server_t::step) and do not
 * change the game state.
 */

#include "live_state.h"
#include "mcp_server.h"

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <map>
#include <vector>

#include "../simworld.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simplan.h"
#include "../simversion.h"
#include "../boden/grund.h"
#include "../obj/simobj.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/route.h"
#include "../io/rdwr/rdwr_stream.h"
#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/vehicle_desc.h"
#include "../utils/simstring.h"


// ---------------------------------------------------------------------------
// Change log
// ---------------------------------------------------------------------------

bool live_changes_t::recording = false;
bool live_changes_t::enabled = false;
uint32 live_changes_t::world_resets = 0;
bool live_changes_t::keep_epoch = false;
uint32 live_changes_t::epoch = 0;

namespace {

/// one entry of the change log: a tile (typ 0), or the state of a signal, roadsign or crossing (typ = its obj_t::typ)
struct change_t
{
	uint32 sync;
	sint16 x, y;
	sint8 z;
	uint8 typ;
	uint8 dir;
	uint8 state;
};

const uint32 LOG_SIZE = 1u << 15;
change_t change_log[LOG_SIZE];
uint32 next_seq = 0;   ///< sequence number of the next entry
uint32 first_seq = 0;  ///< oldest entry still in the log
uint32 read_seq = 0;   ///< next_seq at the last get_changes: entries before it may have been delivered and must not change

/// entries of the same sync looked at for merging a tile
const uint32 MERGE_LOOKBACK = 16;

void push_change(const change_t &c)
{
	change_log[next_seq % LOG_SIZE] = c;
	next_seq++;
	if(  next_seq - first_seq > LOG_SIZE  ) {
		first_seq = next_seq - LOG_SIZE;
	}
}

}


void live_changes_t::record_tile(sint16 x, sint16 y)
{
	karte_t *welt = world();
	if(  !welt  ||  !welt->is_within_limits(x, y)  ) {
		return;
	}
	const uint32 sync = welt->get_sync_steps();
	// the same tile in the same sync step is recorded once (unless the first one was already delivered)
	for(  uint32 seq = next_seq;  seq > first_seq  &&  seq > read_seq  &&  next_seq - seq < MERGE_LOOKBACK;  ) {
		const change_t &c = change_log[--seq % LOG_SIZE];
		if(  c.sync != sync  ) {
			break;
		}
		if(  c.typ == 0  &&  c.x == x  &&  c.y == y  ) {
			return;
		}
	}
	change_t c = { sync, x, y, 0, 0, 0, 0 };
	push_change(c);
}


void live_changes_t::record_obj(const obj_t *o)
{
	// no virtual calls here: objects also come here from obj_t::~obj_t
	if(  o->is_moving()  ||  o->get_flag(obj_t::no_change_log)  ) {
		return;
	}
	record_tile(o->get_pos().x, o->get_pos().y);
}


void live_changes_t::record_sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state)
{
	karte_t *welt = world();
	const koord3d pos = o->get_pos();
	if(  !welt  ||  !welt->is_within_limits(pos.x, pos.y)  ) {
		return;
	}
	change_t c = { welt->get_sync_steps(), pos.x, pos.y, pos.z, typ, dir, state };
	push_change(c);
}


void live_changes_t::set_enabled(bool yesno)
{
	enabled = yesno;
	update_recording();
}


void live_changes_t::update_recording()
{
	recording = enabled  &&  world_resets == 0;
}


live_changes_t::world_reset_t::world_reset_t()
{
	world_resets++;
	update_recording();
}


live_changes_t::world_reset_t::~world_reset_t()
{
	if(  --world_resets == 0  ) {
		if(  keep_epoch  ) {
			keep_epoch = false;
		}
		else {
			// the time keeps the epochs of a restarted game apart
			const uint32 now = (uint32)time(NULL);
			epoch = max(epoch + 1, now);
			first_seq = next_seq;
		}
	}
	update_recording();
}


uint32 live_changes_t::get_epoch()
{
	if(  epoch == 0  ) {
		epoch = (uint32)time(NULL);
	}
	return epoch;
}


// ---------------------------------------------------------------------------
// JSON output
// ---------------------------------------------------------------------------

namespace {

void add_int(std::string &out, sint64 v)
{
	char buf[24];
	const std::to_chars_result r = std::to_chars(buf, buf + sizeof(buf), v);
	out.append(buf, r.ptr - buf);
}

void add_double(std::string &out, double v, int decimals)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%.*f", decimals, v);
	out += buf;
}

void add_str(std::string &out, const char *s)
{
	out += '"';
	out += mcp_server_t::json_escape(s ? s : "");
	out += '"';
}

void add_koord3d(std::string &out, const koord3d &k)
{
	out += '[';
	add_int(out, k.x);
	out += ',';
	add_int(out, k.y);
	out += ',';
	add_int(out, k.z);
	out += ']';
}

std::string error_json(const char *msg)
{
	std::string out = "{\"error\":";
	add_str(out, msg);
	out += '}';
	return out;
}

/// the strings of a JSON array of strings ("[\"a\",\"b\"]")
std::vector<std::string> parse_string_array(const std::string &raw)
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

/// the numbers of a JSON array of integers ("[1,2,3]")
std::vector<sint64> parse_int_array(const std::string &raw)
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

/// an integer argument; false if missing or not a number
bool get_int_arg(const std::string &args_json, const char *key, sint64 &value)
{
	const std::string raw = mcp_server_t::json_get_raw(args_json, key);
	if(  raw.empty()  ||  raw == "null"  ) {
		return false;
	}
	char *end;
	value = strtoll(raw.c_str(), &end, 10);
	return end != raw.c_str()  &&  *end == 0;
}

/// position on the route of the tile the vehicle is on (the route index points at pos_next), -1 if unknown
sint32 route_index_of_own_tile(const vehicle_t *v, const route_t *route)
{
	const uint32 next = v->get_route_index();
	if(  next == 0  ||  next - 1 >= route->get_count()  ) {
		return -1;
	}
	return route->at((uint16)(next - 1)) == v->get_pos() ? (sint32)(next - 1) : -1;
}

/// FNV-1a over the tiles of a route: changes (almost surely) when the route changes
uint32 route_hash(const route_t *route)
{
	uint32 h = 2166136261u;
	for(  uint32 i = 0;  i < route->get_count();  i++  ) {
		const koord3d &k = route->at((uint16)i);
		const sint16 v[3] = { k.x, k.y, k.z };
		for(  int j = 0;  j < 3;  j++  ) {
			h = (h ^ (uint16)v[j]) * 16777619u;
		}
	}
	return h;
}

/// the cargo of a vehicle: [[goods index, amount], ...]
void add_cargo(std::string &out, const vehicle_t *v)
{
	std::map<uint8, uint32> sum;
	for(  const ware_t &w : v->get_cargo()  ) {
		sum[w.get_index()] += w.menge;
	}
	out += '[';
	bool first = true;
	for(  const auto &s : sum  ) {
		if(  !first  ) {
			out += ',';
		}
		first = false;
		out += '[';
		add_int(out, s.first);
		out += ',';
		add_int(out, s.second);
		out += ']';
	}
	out += ']';
}

void add_air(std::string &out, const vehicle_t *v)
{
	const air_vehicle_t *air = v->get_waytype() == air_wt ? static_cast<const air_vehicle_t *>(v) : NULL;
	if(  !air  ) {
		out += "null";
		return;
	}
	out += '[';
	add_int(out, air->get_flight_state());
	out += ',';
	add_int(out, air->get_raw_flying_height());
	out += ',';
	add_int(out, air->get_raw_target_height());
	out += ']';
}


// ---------------------------------------------------------------------------
// get_live_state
// ---------------------------------------------------------------------------

enum vehicle_field_t {
	VF_X, VF_Y, VF_Z, VF_DIR, VF_STEPS, VF_STEPS_NEXT, VF_RI, VF_LOAD, VF_XOFF, VF_YOFF, VF_AIR,
	VF_NX, VF_NY, VF_NZ, VF_PREV_DIR, VF_DX, VF_DY, VF_HOFF, VF_ZOFF_START, VF_ZOFF_END,
	VF_LANE, VF_NEXT_LANE, VF_SPEED_LIMIT, VF_ROUTE_INDEX, VF_PX, VF_PY, VF_PZ,
	VF_LEADING, VF_LAST, VF_CARGO, VF_CAPACITY, VF_OVERTAKING, VF_WITHDRAW, VF_IN_DEPOT
};

struct vehicle_field_name_t {
	const char *name;
	vehicle_field_t field;
};

const vehicle_field_name_t VEHICLE_FIELDS[] = {
	// the default fields
	{ "x", VF_X }, { "y", VF_Y }, { "z", VF_Z },
	{ "dir", VF_DIR },                  // direction (ribi)
	{ "steps", VF_STEPS }, { "steps_next", VF_STEPS_NEXT },
	{ "ri", VF_RI },                    // index of the vehicle's tile on the convoy route, -1 unknown
	{ "load", VF_LOAD },                // total amount of cargo
	{ "xoff", VF_XOFF }, { "yoff", VF_YOFF },
	{ "air", VF_AIR },                  // [flight state, flying height, target height] (as saved), null if no aircraft
	// on request
	{ "nx", VF_NX }, { "ny", VF_NY }, { "nz", VF_NZ },   // pos_next
	{ "prev_dir", VF_PREV_DIR },
	{ "dx", VF_DX }, { "dy", VF_DY },
	{ "hoff", VF_HOFF },
	{ "zoff_start", VF_ZOFF_START }, { "zoff_end", VF_ZOFF_END },
	{ "lane", VF_LANE }, { "next_lane", VF_NEXT_LANE },
	{ "speed_limit", VF_SPEED_LIMIT },
	{ "route_index", VF_ROUTE_INDEX },  // as in the game: the index of pos_next
	{ "px", VF_PX }, { "py", VF_PY }, { "pz", VF_PZ },   // pos_prev (road vehicles, else -1)
	{ "leading", VF_LEADING }, { "last", VF_LAST },
	{ "cargo", VF_CARGO },              // [[goods index, amount], ...]
	{ "capacity", VF_CAPACITY },
	// of the convoy
	{ "overtaking", VF_OVERTAKING },    // tiles_overtaking
	{ "withdraw", VF_WITHDRAW },
	{ "in_depot", VF_IN_DEPOT }
};
const int NUM_VEHICLE_FIELDS = sizeof(VEHICLE_FIELDS) / sizeof(VEHICLE_FIELDS[0]);
const int NUM_DEFAULT_FIELDS = VF_AIR + 1;

void add_vehicle_field(std::string &out, vehicle_field_t f, const convoi_t *cnv, vehicle_t *v, const route_t *route)
{
	switch(  f  ) {
		case VF_X:            add_int(out, v->get_pos().x); break;
		case VF_Y:            add_int(out, v->get_pos().y); break;
		case VF_Z:            add_int(out, v->get_pos().z); break;
		case VF_DIR:          add_int(out, v->get_direction()); break;
		case VF_STEPS:        add_int(out, v->get_steps()); break;
		case VF_STEPS_NEXT:   add_int(out, v->get_steps_next()); break;
		case VF_RI:           add_int(out, route_index_of_own_tile(v, route)); break;
		case VF_LOAD:         add_int(out, v->get_total_cargo()); break;
		case VF_XOFF:         add_int(out, v->get_xoff()); break;
		case VF_YOFF:         add_int(out, v->get_yoff()); break;
		case VF_AIR:          add_air(out, v); break;
		case VF_NX:           add_int(out, v->get_pos_next().x); break;
		case VF_NY:           add_int(out, v->get_pos_next().y); break;
		case VF_NZ:           add_int(out, v->get_pos_next().z); break;
		case VF_PREV_DIR:     add_int(out, v->get_previous_direction()); break;
		case VF_DX:           add_int(out, v->get_dx()); break;
		case VF_DY:           add_int(out, v->get_dy()); break;
		case VF_HOFF:         add_int(out, v->get_hoff()); break;
		case VF_ZOFF_START:   add_int(out, v->get_zoff_start()); break;
		case VF_ZOFF_END:     add_int(out, v->get_zoff_end()); break;
		case VF_LANE:         add_int(out, v->get_disp_lane()); break;
		case VF_NEXT_LANE:    add_int(out, v->get_next_lane()); break;
		case VF_SPEED_LIMIT:  add_int(out, v->get_speed_limit()); break;
		case VF_ROUTE_INDEX:  add_int(out, v->get_route_index()); break;
		case VF_PX:
		case VF_PY:
		case VF_PZ: {
			koord3d prev = koord3d::invalid;
			if(  v->get_waytype() == road_wt  ) {
				prev = static_cast<const road_vehicle_t *>(v)->get_pos_prev();
			}
			add_int(out, f == VF_PX ? prev.x : f == VF_PY ? prev.y : prev.z);
			break;
		}
		case VF_LEADING:      add_int(out, v->is_leading()); break;
		case VF_LAST:         add_int(out, v->is_last()); break;
		case VF_CARGO:        add_cargo(out, v); break;
		case VF_CAPACITY:     add_int(out, v->get_cargo_max()); break;
		case VF_OVERTAKING:   add_int(out, cnv->get_tiles_overtaking()); break;
		case VF_WITHDRAW:     add_int(out, cnv->get_withdraw()); break;
		case VF_IN_DEPOT:     add_int(out, cnv->in_depot()); break;
	}
}

/// waiting goods of every halt: "halts":[[id, owner, total, [[via halt id, goods index, amount], ...]], ...],"goods":[names]
void add_halts(std::string &out)
{
	out += ",\"halts\":[";
	bool first_halt = true;
	std::map<std::pair<uint32, uint8>, uint32> via;
	for(  const halthandle_t &halt : haltestelle_t::get_alle_haltestellen()  ) {
		if(  !halt.is_bound()  ) {
			continue;
		}
		via.clear();
		uint32 total = 0;
		halt->for_each_waiting_ware( [&](const ware_t &w) {
			via[std::make_pair(w.get_zwischenziel().get_id(), w.get_index())] += w.menge;
			total += w.menge;
		} );
		if(  !first_halt  ) {
			out += ',';
		}
		first_halt = false;
		out += '[';
		add_int(out, halt.get_id());
		out += ',';
		add_int(out, halt->get_owner() ? halt->get_owner()->get_player_nr() : -1);
		out += ',';
		add_int(out, total);
		out += ",[";
		bool first = true;
		for(  const auto &v : via  ) {
			if(  !first  ) {
				out += ',';
			}
			first = false;
			out += '[';
			add_int(out, v.first.first);
			out += ',';
			add_int(out, v.first.second);
			out += ',';
			add_int(out, v.second);
			out += ']';
		}
		out += "]]";
	}
	out += "],\"goods\":[";
	for(  uint16 i = 0;  i < goods_manager_t::get_count();  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		add_str(out, goods_manager_t::get_info(i)->get_name());
	}
	out += ']';
}

/// fields of the start of every answer: epoch and time
void add_head(std::string &out, karte_t *welt)
{
	out += "\"epoch\":";
	add_int(out, live_changes_t::get_epoch());
	out += ",\"sync\":";
	add_int(out, welt->get_sync_steps());
}

} // namespace


std::string live_state::get_live_state(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	const auto t0 = std::chrono::steady_clock::now();

	// the vehicle fields asked for
	std::vector<int> fields;
	const std::string fields_raw = mcp_server_t::json_get_raw(args_json, "fields");
	if(  !fields_raw.empty()  &&  fields_raw != "null"  ) {
		for(  const std::string &name : parse_string_array(fields_raw)  ) {
			int found = -1;
			for(  int i = 0;  i < NUM_VEHICLE_FIELDS;  i++  ) {
				if(  name == VEHICLE_FIELDS[i].name  ) {
					found = i;
					break;
				}
			}
			if(  found < 0  ) {
				return error_json(("unknown field: " + name).c_str());
			}
			fields.push_back(found);
		}
	}
	else {
		for(  int i = 0;  i < NUM_DEFAULT_FIELDS;  i++  ) {
			fields.push_back(i);
		}
	}
	const bool with_halts = mcp_server_t::json_get_raw(args_json, "halts") == "true";

	// routes are most of the answer: they can be left out (rt null), sent for some convoys only, and replaced by hashes
	const bool all_routes = mcp_server_t::json_get_raw(args_json, "routes") != "false";
	const std::string route_ids_raw = mcp_server_t::json_get_raw(args_json, "route_ids");
	const bool some_routes = !route_ids_raw.empty()  &&  route_ids_raw != "null";
	std::vector<sint64> route_ids;
	if(  some_routes  ) {
		route_ids = parse_int_array(route_ids_raw);
		std::sort(route_ids.begin(), route_ids.end());
	}
	const bool with_route_hash = mcp_server_t::json_get_raw(args_json, "route_hash") == "true";
	std::string route_hashes;

	// frames per second of sync_steps (fixed in network games)
	sint64 fps = env_t::fps;
	if(  env_t::networkmode  ) {
		fps = 1000 / (1000 / clamp(welt->get_settings().get_frames_per_second(), env_t::min_fps, env_t::max_fps));
	}

	std::string out;
	out.reserve(1 << 20);
	out += "{\"schema\":1,";
	add_head(out, welt);
	out += ",\"fps\":";
	add_int(out, fps);
	out += ",\"ahead\":";
	add_int(out, env_t::server_frames_ahead + env_t::additional_client_frames_behind);
	out += ",\"paused\":";
	out += welt->is_paused() ? "true" : "false";
	out += ",\"vf\":[";
	for(  size_t i = 0;  i < fields.size();  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		add_str(out, VEHICLE_FIELDS[fields[i]].name);
	}
	out += "],\"fields\":[";
	for(  int i = 0;  i < NUM_VEHICLE_FIELDS;  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		add_str(out, VEHICLE_FIELDS[i].name);
	}
	out += "],\"c\":[";

	// [id, owner, state, speed (km/h), reversed, rt0, rt, vehicles]
	bool first_cnv = true;
	for(  const convoihandle_t &cnv : welt->convoys()  ) {
		if(  !cnv.is_bound()  ) {
			continue;
		}
		if(  !first_cnv  ) {
			out += ',';
		}
		first_cnv = false;
		out += '[';
		add_int(out, cnv.get_id());
		out += ',';
		add_int(out, cnv->get_owner() ? cnv->get_owner()->get_player_nr() : -1);
		out += ',';
		add_int(out, cnv->get_state());
		out += ',';
		add_double(out, cnv->get_akt_speed() * (double)VEHICLE_SPEED_FACTOR / 64.0, 1);
		out += ',';
		add_int(out, cnv->is_reversed());
		// the whole route, so rt0 is always 0
		const route_t *route = cnv->get_route();
		out += ",0,";
		if(  some_routes ? std::binary_search(route_ids.begin(), route_ids.end(), (sint64)cnv.get_id()) : all_routes  ) {
			out += '[';
			for(  uint32 i = 0;  i < route->get_count();  i++  ) {
				const koord3d &k = route->at((uint16)i);
				if(  i > 0  ) {
					out += ',';
				}
				add_int(out, k.x);
				out += ',';
				add_int(out, k.y);
				out += ',';
				add_int(out, k.z);
			}
			out += ']';
		}
		else {
			out += "null";
		}
		if(  with_route_hash  ) {
			if(  !route_hashes.empty()  ) {
				route_hashes += ',';
			}
			add_int(route_hashes, route_hash(route));
		}
		out += ",[";
		// in the order of the savegame (front() first)
		for(  uint8 i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			vehicle_t *v = cnv->get_vehikel(i);
			if(  i > 0  ) {
				out += ',';
			}
			out += '[';
			for(  size_t f = 0;  f < fields.size();  f++  ) {
				if(  f > 0  ) {
					out += ',';
				}
				add_vehicle_field(out, VEHICLE_FIELDS[fields[f]].field, cnv.get_rep(), v, route);
			}
			out += ']';
		}
		out += "]]";
	}
	out += ']';

	if(  with_route_hash  ) {
		out += ",\"rh\":[" + route_hashes + "]";
	}

	if(  with_halts  ) {
		add_halts(out);
	}

	const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
	out += ",\"gen_ms\":";
	add_double(out, ms, 3);
	out += '}';
	return out;
}


// ---------------------------------------------------------------------------
// get_tiles
// ---------------------------------------------------------------------------

namespace {

/// a stream that collects what is written into a string
class string_rdwr_stream_t : public rdwr_stream_t
{
public:
	explicit string_rdwr_stream_t(std::string &out) : rdwr_stream_t(true), out(out) { status = STATUS_OK; }

	size_t read(void *, size_t) OVERRIDE { return 0; }

	size_t write(const void *buf, size_t len) OVERRIDE
	{
		out.append((const char *)buf, len);
		return len;
	}

private:
	std::string &out;
};

/// saves into a string in the uncompressed savegame format of this version (no header)
class string_saver_t : public loadsave_t
{
public:
	explicit string_saver_t(std::string &out)
	{
		mode = binary;
		buffered = false;
		stream = new string_rdwr_stream_t(out);
		const combined_version v = int_version(SAVEGAME_VER_NR, NULL);
		finfo.version = v.version;
		finfo.OTRP_version = v.OTRP_version;
		// as wr_open does: the last part of the pak path
		std::string pak = env_t::objfilename;
		while(  !pak.empty()  &&  (pak.back() == '/'  ||  pak.back() == '\\')  ) {
			pak.pop_back();
		}
		const size_t sep = pak.find_last_of("/\\:");
		tstrncpy(finfo.pak_extension, sep == std::string::npos ? pak.c_str() : pak.c_str() + sep + 1, lengthof(finfo.pak_extension));
	}
};

const char BASE64_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void add_base64(std::string &out, const std::string &data)
{
	for(  size_t i = 0;  i < data.size();  i += 3  ) {
		const uint32 a = (uint8)data[i];
		const uint32 b = i + 1 < data.size() ? (uint8)data[i + 1] : 0;
		const uint32 c = i + 2 < data.size() ? (uint8)data[i + 2] : 0;
		const uint32 value = (a << 16) | (b << 8) | c;
		out += BASE64_CHARS[(value >> 18) & 0x3F];
		out += BASE64_CHARS[(value >> 12) & 0x3F];
		out += i + 1 < data.size() ? BASE64_CHARS[(value >> 6) & 0x3F] : '=';
		out += i + 2 < data.size() ? BASE64_CHARS[value & 0x3F] : '=';
	}
}

const sint64 MAX_TILES = 4096;

} // namespace


std::string live_state::get_tiles(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	sint64 x0, y0, w, h;
	if(  !get_int_arg(args_json, "x0", x0)  ||  !get_int_arg(args_json, "y0", y0)  ||  !get_int_arg(args_json, "w", w)  ||  !get_int_arg(args_json, "h", h)  ) {
		return error_json("x0, y0, w and h are required integers");
	}
	if(  w <= 0  ||  h <= 0  ||  w * h > MAX_TILES  ) {
		return error_json("bad rectangle (w*h <= 4096)");
	}
	if(  x0 < 0  ||  y0 < 0  ||  x0 + w > welt->get_size().x  ||  y0 + h > welt->get_size().y  ) {
		return error_json("rectangle outside the map");
	}

	// the same as karte_t::rdwr_gamestate writes: the tiles (y outside, x inside), then the climate bytes of the rectangle
	std::string data;
	string_saver_t file(data);
	for(  sint16 y = (sint16)y0;  y < y0 + h;  y++  ) {
		for(  sint16 x = (sint16)x0;  x < x0 + w;  x++  ) {
			welt->access(x, y)->rdwr(&file, koord(x, y));
		}
	}
	if(  file.is_version_atleast(121, 1)  ) {
		for(  sint16 y = (sint16)y0;  y < y0 + h;  y++  ) {
			for(  sint16 x = (sint16)x0;  x < x0 + w;  x++  ) {
				uint8 cl = welt->get_default_climate(koord(x, y));
				file.rdwr_byte(cl);
			}
		}
	}

	std::string out = "{";
	add_head(out, welt);
	out += ",\"version\":";
	add_int(out, file.get_version_int());
	out += ",\"otrp\":";
	add_int(out, file.get_OTRP_version());
	out += ",\"x0\":";
	add_int(out, x0);
	out += ",\"y0\":";
	add_int(out, y0);
	out += ",\"w\":";
	add_int(out, w);
	out += ",\"h\":";
	add_int(out, h);
	out += ",\"data\":\"";
	file.close();
	add_base64(out, data);
	out += "\"}";
	return out;
}


// ---------------------------------------------------------------------------
// get_changes
// ---------------------------------------------------------------------------

std::string live_changes_t::get_changes_json(karte_t *welt, bool has_since, uint32 since)
{
	const bool overflow = has_since  &&  (since < first_seq  ||  since > next_seq);
	read_seq = next_seq;

	std::string out = "{";
	add_head(out, welt);
	out += ",\"seq\":";
	add_int(out, next_seq);
	out += ",\"floor\":";
	add_int(out, first_seq);
	out += ",\"overflow\":";
	out += overflow ? "true" : "false";

	std::string tiles, sig;
	if(  has_since  &&  !overflow  ) {
		for(  uint32 seq = since;  seq != next_seq;  seq++  ) {
			const change_t &c = change_log[seq % LOG_SIZE];
			std::string &list = c.typ == 0 ? tiles : sig;
			if(  !list.empty()  ) {
				list += ',';
			}
			list += '[';
			add_int(list, c.sync);
			list += ',';
			add_int(list, c.x);
			list += ',';
			add_int(list, c.y);
			if(  c.typ != 0  ) {
				// [sync, x, y, z, typ, dir, state]
				list += ',';
				add_int(list, c.z);
				list += ',';
				add_int(list, c.typ);
				list += ',';
				add_int(list, c.dir);
				list += ',';
				add_int(list, c.state);
			}
			list += ']';
		}
	}
	out += ",\"tiles\":[" + tiles + "],\"sig\":[" + sig + "]}";
	return out;
}


std::string live_state::get_changes(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	sint64 since = 0;
	const bool has_since = get_int_arg(args_json, "since", since);
	if(  has_since  &&  (since < 0  ||  since > 0xFFFFFFFFll)  ) {
		return error_json("bad since");
	}
	return live_changes_t::get_changes_json(welt, has_since, (uint32)since);
}


// ---------------------------------------------------------------------------
// dump_vehicles
// ---------------------------------------------------------------------------

namespace {

const size_t MAX_DUMP_CONVOYS = 50;

void add_named_int(std::string &out, const char *name, sint64 v)
{
	out += ",\"";
	out += name;
	out += "\":";
	add_int(out, v);
}

void add_named_koord3d(std::string &out, const char *name, const koord3d &k)
{
	out += ",\"";
	out += name;
	out += "\":";
	add_koord3d(out, k);
}

void dump_vehicle(std::string &out, vehicle_t *v, const route_t *route)
{
	out += "{\"typ\":";
	add_int(out, v->get_typ());
	out += ",\"name\":";
	add_str(out, v->get_desc()->get_name());
	add_named_koord3d(out, "pos", v->get_pos());
	add_named_koord3d(out, "pos_next", v->get_pos_next());
	if(  v->get_waytype() == road_wt  ) {
		add_named_koord3d(out, "pos_prev", static_cast<const road_vehicle_t *>(v)->get_pos_prev());
	}
	add_named_int(out, "direction", v->get_direction());
	add_named_int(out, "prev_direction", v->get_previous_direction());
	add_named_int(out, "steps", v->get_steps());
	add_named_int(out, "steps_next", v->get_steps_next());
	add_named_int(out, "dx", v->get_dx());
	add_named_int(out, "dy", v->get_dy());
	add_named_int(out, "hoff", v->get_hoff());
	add_named_int(out, "xoff", v->get_xoff());
	add_named_int(out, "yoff", v->get_yoff());
	add_named_int(out, "zoff_start", v->get_zoff_start());
	add_named_int(out, "zoff_end", v->get_zoff_end());
	add_named_int(out, "disp_lane", v->get_disp_lane());
	add_named_int(out, "next_lane", v->get_next_lane());
	add_named_int(out, "speed_limit", v->get_speed_limit());
	add_named_int(out, "route_index", v->get_route_index());
	add_named_int(out, "ri", route_index_of_own_tile(v, route));
	add_named_int(out, "leading", v->is_leading());
	add_named_int(out, "last", v->is_last());
	add_named_int(out, "total_cargo", v->get_total_cargo());
	add_named_int(out, "capacity", v->get_cargo_max());
	out += ",\"cargo\":";
	add_cargo(out, v);
	out += ",\"air\":";
	add_air(out, v);
	out += '}';
}

} // namespace


std::string live_state::dump_vehicles(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	const std::vector<sint64> ids = parse_int_array(mcp_server_t::json_get_raw(args_json, "ids"));
	if(  ids.empty()  ||  ids.size() > MAX_DUMP_CONVOYS  ) {
		return error_json("ids: 1 to 50 convoy ids");
	}

	std::string out = "{";
	add_head(out, welt);
	add_named_int(out, "diagonal_steps", vehicle_base_t::get_diagonal_vehicle_steps_per_tile());
	out += ",\"convoys\":[";
	bool first_cnv = true;
	for(  sint64 id : ids  ) {
		convoihandle_t cnv;
		for(  const convoihandle_t &c : welt->convoys()  ) {
			if(  c.is_bound()  &&  (sint64)c.get_id() == id  ) {
				cnv = c;
				break;
			}
		}
		if(  !first_cnv  ) {
			out += ',';
		}
		first_cnv = false;
		if(  !cnv.is_bound()  ) {
			out += "{\"id\":";
			add_int(out, id);
			out += ",\"error\":\"no such convoy\"}";
			continue;
		}
		out += "{\"id\":";
		add_int(out, id);
		out += ",\"name\":";
		add_str(out, cnv->get_name());
		add_named_int(out, "owner", cnv->get_owner() ? cnv->get_owner()->get_player_nr() : -1);
		add_named_int(out, "state", cnv->get_state());
		add_named_int(out, "akt_speed", cnv->get_akt_speed());
		out += ",\"speed_kmh\":";
		add_double(out, cnv->get_akt_speed() * (double)VEHICLE_SPEED_FACTOR / 64.0, 2);
		add_named_int(out, "reversed", cnv->is_reversed());
		add_named_int(out, "withdraw", cnv->get_withdraw());
		add_named_int(out, "in_depot", cnv->in_depot());
		add_named_int(out, "tiles_overtaking", cnv->get_tiles_overtaking());
		add_named_int(out, "line", cnv->get_line().is_bound() ? (sint64)cnv->get_line().get_id() : -1);
		const route_t *route = cnv->get_route();
		out += ",\"route\":[";
		for(  uint32 i = 0;  i < route->get_count();  i++  ) {
			if(  i > 0  ) {
				out += ',';
			}
			add_koord3d(out, route->at((uint16)i));
		}
		out += "],\"vehicles\":[";
		for(  uint8 i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			if(  i > 0  ) {
				out += ',';
			}
			dump_vehicle(out, cnv->get_vehikel(i), route);
		}
		out += "]}";
	}
	out += "]}";
	return out;
}
