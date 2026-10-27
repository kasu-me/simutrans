/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * Read-only MCP tools for a live view of the game (a web viewer bridge).
 *
 * get_live_state  every convoy and vehicle as it is now; with `since` only
 *                 what changed (see live_util::entity_changes_t), and on
 *                 request halts, factories, cities, players, depots, lines
 *                 (live_world.cc)
 * get_tiles       a rectangle of tiles in the savegame format
 * get_changes     the tiles and signal states changed since a sequence number
 * dump_vehicles   all fields of some convoys, for diagnosis
 *
 * All of them run on the main thread (from mcp_server_t::step) and do not
 * change the game state (what they remember for `since` and `rlap` is their own).
 */

#include "live_state.h"
#include "live_util.h"
#include "mcp_server.h"

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <charconv>
#include <chrono>
#include <map>
#include <unordered_map>
#include <vector>

#include "../simworld.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simplan.h"
#include "../simversion.h"
#include "../boden/grund.h"
#include "../obj/simobj.h"
#include "../obj/gebaeude.h"
#include "../obj/crossing.h"
#include "../descriptor/building_desc.h"
#include "../descriptor/crossing_desc.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/route.h"
#include "../dataobj/schedule.h"
#include "../io/rdwr/rdwr_stream.h"
#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/vehicle_desc.h"
#include "../utils/simstring.h"
#include "../simcolor.h"
#include "../sys/simsys.h"
#include "../player/finance.h"

using live_util::add_int;
using live_util::add_double;
using live_util::add_str;
using live_util::error_json;
using live_util::get_int_arg;
using live_util::get_bool_arg;
using live_util::parse_string_array;
using live_util::parse_int_array;


// ---------------------------------------------------------------------------
// Change log
// ---------------------------------------------------------------------------

bool live_changes_t::recording = false;
const obj_t *live_changes_t::destroying = NULL;
bool live_changes_t::enabled = false;
uint32 live_changes_t::world_resets = 0;
bool live_changes_t::keep_epoch = false;
uint32 live_changes_t::epoch = 0;
const char *live_changes_t::busy_reason = NULL;
int live_changes_t::joining = 0;
std::vector<live_changes_t::tool_result_t> live_changes_t::tool_results;

namespace {

/// typ of the entries of the change log that are events (not tiles or signals)
const uint8 CHANGE_SOLD = 255;
const uint8 CHANGE_ENTITY = 254;
const uint8 CHANGE_SAVED = 253;
const uint8 CHANGE_MONTH = 252;

/// the "type" of the entity events, by live_changes_t::entity_kind_t
const char *const ENTITY_TYPES[live_changes_t::K_MAX] = { "station", "convoy", "line", "factory", "city", "player", "depot" };

/// the names of the bits of "what", by bit
const char *const WHAT_NAMES[live_changes_t::W_MAX_BIT] = { "new", "gone", "name", "tiles", "line", "schedule", "vehicles", "colour", "fields", "settings" };

/**
 * One entry of the change log: a tile (typ 0, id = kind bits), the state of a signal, roadsign
 * or crossing (typ = its obj_t::typ), or an event:
 * CHANGE_SOLD (id = convoy, dir = player, value = amount),
 * CHANGE_ENTITY (dir = entity_kind_t, id or x, y, z, value = what bits),
 * CHANGE_SAVED (value = ticks, file name in saved_files),
 * CHANGE_MONTH (id = month counter, value = ticks).
 */
struct change_t
{
	uint32 sync;
	sint16 x, y;
	sint8 z;
	uint8 typ;
	uint8 dir;
	uint8 state;
	uint32 id;
	sint64 value;
};

const uint32 LOG_SIZE = 1u << 15;
change_t change_log[LOG_SIZE];
uint32 next_seq = 0;   ///< sequence number of the next entry
uint32 first_seq = 0;  ///< oldest entry still in the log
uint32 read_seq = 0;   ///< next_seq at the last get_changes: entries before it may have been delivered and must not change

/// entries of the same sync looked at for merging a tile
const uint32 MERGE_LOOKBACK = 16;

/// the file names of the CHANGE_SAVED entries, by sequence number
std::map<uint32, std::string> saved_files;

void push_change(const change_t &c)
{
	change_log[next_seq % LOG_SIZE] = c;
	next_seq++;
	if(  next_seq - first_seq > LOG_SIZE  ) {
		first_seq = next_seq - LOG_SIZE;
	}
	while(  !saved_files.empty()  &&  saved_files.begin()->first < first_seq  ) {
		saved_files.erase(saved_files.begin());
	}
}

/// one message of the game (get_messages)
struct message_entry_t
{
	uint32 seq;
	uint32 sync;
	sint32 type;
	sint16 x, y;
	sint16 player;
	sint32 month;
	std::string text;
};

const size_t MESSAGE_LOG_SIZE = 1024;
std::vector<message_entry_t> message_log;   ///< ring of MESSAGE_LOG_SIZE
uint32 message_next_seq = 0;
uint32 message_first_seq = 0;

void add_event(std::string &out, const change_t &c, uint32 seq);

}


void live_changes_t::record_tile(sint16 x, sint16 y, uint16 kind)
{
	karte_t *welt = world();
	if(  !welt  ||  !welt->is_within_limits(x, y)  ) {
		return;
	}
	const uint32 sync = welt->get_sync_steps();
	// the same tile in the same sync step is recorded once (unless the first one was already delivered)
	for(  uint32 seq = next_seq;  seq > first_seq  &&  seq > read_seq  &&  next_seq - seq < MERGE_LOOKBACK;  ) {
		change_t &c = change_log[--seq % LOG_SIZE];
		if(  c.sync != sync  ) {
			break;
		}
		if(  c.typ == 0  &&  c.x == x  &&  c.y == y  ) {
			c.id |= kind;
			return;
		}
	}
	change_t c = { sync, x, y, 0, 0, 0, 0, kind, 0 };
	push_change(c);
}


uint16 live_changes_t::way_kind(waytype_t wt)
{
	switch(  wt  ) {
		case road_wt:        return T_ROAD;
		case track_wt:
		case tram_wt:
		case monorail_wt:
		case maglev_wt:
		case narrowgauge_wt: return T_TRACK;
		case water_wt:       return T_WATER;
		case air_wt:         return T_AIR;
		default:             return T_OTHER;
	}
}


namespace {

/// the kind of a building: a transport building (stop, depot ...) or another one, 0 while it gets its first tile
uint16 building_kind(const gebaeude_t *gb)
{
	const building_tile_desc_t *tile = gb->get_tile();
	if(  tile == NULL  ) {
		return 0;
	}
	return tile->get_desc()->is_transport_building() ? live_changes_t::T_STATION : live_changes_t::T_BUILDING;
}

/// the kind of a change of an object that is not being destroyed
uint16 obj_kind(const obj_t *o)
{
	switch(  o->get_typ()  ) {
		case obj_t::way:
		case obj_t::bruecke:
		case obj_t::tunnel:
			return live_changes_t::way_kind(o->get_waytype());
		case obj_t::signal:
		case obj_t::roadsign:
			return live_changes_t::T_SIGN;
		case obj_t::crossing: {
			const crossing_desc_t *desc = static_cast<const crossing_t *>(o)->get_desc();
			return desc ? live_changes_t::way_kind(desc->get_waytype(0)) | live_changes_t::way_kind(desc->get_waytype(1)) : live_changes_t::T_OTHER;
		}
		case obj_t::gebaeude:
			return building_kind(static_cast<const gebaeude_t *>(o));
		case obj_t::bahndepot:
		case obj_t::strassendepot:
		case obj_t::schiffdepot:
		case obj_t::airdepot:
		case obj_t::monoraildepot:
		case obj_t::tramdepot:
		case obj_t::maglevdepot:
		case obj_t::narrowgaugedepot:
			return live_changes_t::T_STATION;
		default:
			return live_changes_t::T_OTHER;
	}
}

}


void live_changes_t::record_obj(const obj_t *o, bool typed)
{
	if(  o->is_moving()  ||  o->get_flag(obj_t::no_change_log)  ) {
		return;
	}
	// no virtual calls on an object in obj_t::~obj_t (its class reported it with obj_destroyed, if at all)
	record_tile(o->get_pos().x, o->get_pos().y, typed  &&  o != destroying ? obj_kind(o) : 0);
}


void live_changes_t::obj_destroyed(const obj_t *o, uint16 kind)
{
	if(  recording  &&  !o->get_flag(obj_t::not_on_map)  &&  !o->get_flag(obj_t::no_change_log)  ) {
		record_tile(o->get_pos().x, o->get_pos().y, kind);
	}
}


void live_changes_t::record_sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state)
{
	karte_t *welt = world();
	const koord3d pos = o->get_pos();
	if(  !welt  ||  !welt->is_within_limits(pos.x, pos.y)  ) {
		return;
	}
	change_t c = { welt->get_sync_steps(), pos.x, pos.y, pos.z, typ, dir, state, 0, 0 };
	push_change(c);
}


void live_changes_t::record_sold(uint32 convoy, uint8 player_nr, sint64 amount)
{
	karte_t *welt = world();
	if(  !welt  ) {
		return;
	}
	change_t c = { welt->get_sync_steps(), 0, 0, 0, CHANGE_SOLD, player_nr, 0, convoy, amount };
	push_change(c);
}


void live_changes_t::record_entity(entity_kind_t kind, uint32 id, koord3d pos, uint16 what)
{
	karte_t *welt = world();
	if(  !welt  ) {
		return;
	}
	const uint32 sync = welt->get_sync_steps();
	// the same entity in the same sync step is one event (unless the first one was already delivered)
	for(  uint32 seq = next_seq;  seq > first_seq  &&  seq > read_seq  &&  next_seq - seq < MERGE_LOOKBACK;  ) {
		change_t &c = change_log[--seq % LOG_SIZE];
		if(  c.sync != sync  ) {
			break;
		}
		if(  c.typ == CHANGE_ENTITY  &&  c.dir == kind  &&  c.id == id  &&  c.x == pos.x  &&  c.y == pos.y  &&  c.z == pos.z  ) {
			c.value |= what;
			return;
		}
	}
	change_t c = { sync, pos.x, pos.y, pos.z, CHANGE_ENTITY, (uint8)kind, 0, id, what };
	push_change(c);
}


void live_changes_t::record_saved(const char *filename)
{
	karte_t *welt = world();
	if(  !welt  ) {
		return;
	}
	saved_files[next_seq] = filename ? filename : "";
	change_t c = { welt->get_sync_steps(), 0, 0, 0, CHANGE_SAVED, 0, 0, 0, (sint64)welt->get_ticks() };
	push_change(c);
}


void live_changes_t::record_month()
{
	karte_t *welt = world();
	if(  !welt  ) {
		return;
	}
	change_t c = { welt->get_sync_steps(), 0, 0, 0, CHANGE_MONTH, 0, 0, welt->get_current_month(), (sint64)welt->get_ticks() };
	push_change(c);
}


void live_changes_t::record_message(const char *text, sint32 type, koord pos, uint32 color)
{
	karte_t *welt = world();
	if(  !welt  ) {
		return;
	}
	if(  message_log.size() < MESSAGE_LOG_SIZE  ) {
		message_log.resize(MESSAGE_LOG_SIZE);
	}
	message_entry_t &m = message_log[message_next_seq % MESSAGE_LOG_SIZE];
	m.seq = message_next_seq;
	m.sync = welt->get_sync_steps();
	m.type = type;
	m.x = pos.x;
	m.y = pos.y;
	// a message of a player has its colour (PLAYER_FLAG | player number)
	m.player = (color & PLAYER_FLAG) ? (sint16)(color & ~PLAYER_FLAG) : -1;
	m.month = welt->get_current_month();
	m.text = text ? text : "";
	message_next_seq++;
	if(  message_next_seq - message_first_seq > MESSAGE_LOG_SIZE  ) {
		message_first_seq = message_next_seq - MESSAGE_LOG_SIZE;
	}
}


/// the cash of a player (1/100 credits), 0 if there is no such player
static sint64 account_balance(karte_t *welt, uint8 nr)
{
	const player_t *player = welt ? welt->get_player(nr) : NULL;
	return player ? player->get_finance()->get_account_balance() : 0;
}


live_changes_t::tool_op_t::tool_op_t(const player_t *player) :
	player(player),
	seq(next_seq),
	active(recording)
{
	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		balance[i] = active ? account_balance(world(), i) : 0;
	}
}


void live_changes_t::tool_op_t::done(const void *vm, uint16 tool_id, const char *default_param, const char *err)
{
	karte_t *welt = world();
	if(  !active  ||  !recording  ||  !welt  ) {
		return;
	}
	std::string out = "{\"tool\":";
	add_int(out, tool_id);
	out += ",\"param\":";
	add_str(out, default_param);
	out += ",\"sync\":";
	add_int(out, welt->get_sync_steps());
	out += ",\"player\":";
	add_int(out, player ? player->get_player_nr() : -1);
	// the change of the cash of the acting player, and of every player whose cash changed
	out += ",\"cost\":";
	add_double(out, player ? (account_balance(welt, player->get_player_nr()) - balance[player->get_player_nr()]) / 100.0 : 0.0, 2);
	out += ",\"cash\":[";
	bool first_cash = true;
	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		const sint64 delta = account_balance(welt, i) - balance[i];
		if(  delta != 0  ) {
			if(  !first_cash  ) {
				out += ',';
			}
			first_cash = false;
			out += '[';
			add_int(out, i);
			out += ',';
			add_double(out, delta / 100.0, 2);
			out += ']';
		}
	}
	out += "],\"err\":";
	if(  err  ) {
		add_str(out, err);
	}
	else {
		out += "null";
	}
	// what the change log got meanwhile (lost if the log overflowed)
	const bool lost = seq < first_seq;
	std::string tiles, events;
	std::vector<std::pair<sint16, sint16>> seen;
	for(  uint32 s = lost ? first_seq : seq;  s != next_seq;  s++  ) {
		const change_t &c = change_log[s % LOG_SIZE];
		if(  c.typ == 0  ) {
			const std::pair<sint16, sint16> xy(c.x, c.y);
			if(  std::find(seen.begin(), seen.end(), xy) != seen.end()  ) {
				continue;
			}
			seen.push_back(xy);
			if(  !tiles.empty()  ) {
				tiles += ',';
			}
			tiles += '[';
			add_int(tiles, c.x);
			tiles += ',';
			add_int(tiles, c.y);
			tiles += ']';
		}
		else if(  c.typ == CHANGE_ENTITY  ||  c.typ == CHANGE_SOLD  ) {
			if(  !events.empty()  ) {
				events += ',';
			}
			add_event(events, c, s);
		}
	}
	out += ",\"tiles\":[" + tiles + "],\"events\":[" + events + "],\"lost\":";
	out += lost ? "true" : "false";
	out += '}';

	// results nobody asks for are dropped
	if(  tool_results.size() >= 256  ) {
		tool_results.erase(tool_results.begin());
	}
	tool_results.push_back(tool_result_t{ vm, out });
}


live_changes_t::busy_t::busy_t(const char *reason) :
	outer(busy_reason)
{
	busy_reason = reason;
	if(  strcmp(reason, "joining") == 0  ) {
		joining++;
	}
}


live_changes_t::busy_t::~busy_t()
{
	if(  strcmp(busy_reason, "joining") == 0  ) {
		joining--;
	}
	busy_reason = outer;
}


void live_changes_t::busy_poll_intern()
{
	// every 20 ms is enough, and costs the save almost nothing
	static uint32 last = 0;
	const uint32 now = dr_time();
	if(  now - last < 20  ) {
		return;
	}
	last = now;
	mcp_server_t::step_busy(busy_reason);
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
			message_first_seq = message_next_seq;
			forget_world();
		}
	}
	update_recording();
}


uint32 live_changes_t::get_next_seq()
{
	return next_seq;
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

/// an event of the change log as a JSON object (get_changes "events")
void add_event(std::string &out, const change_t &c, uint32 seq)
{
	switch(  c.typ  ) {
		case CHANGE_SOLD:
			out += "{\"type\":\"sold\",\"sync\":";
			add_int(out, c.sync);
			out += ",\"convoy\":";
			add_int(out, c.id);
			out += ",\"player\":";
			add_int(out, c.dir);
			out += ",\"amount\":";
			add_double(out, c.value / 100.0, 2);
			out += '}';
			break;

		case CHANGE_ENTITY: {
			const live_changes_t::entity_kind_t kind = (live_changes_t::entity_kind_t)c.dir;
			out += "{\"type\":\"";
			out += ENTITY_TYPES[kind];
			out += "\",\"sync\":";
			add_int(out, c.sync);
			if(  kind == live_changes_t::K_FACTORY  ||  kind == live_changes_t::K_CITY  ||  kind == live_changes_t::K_DEPOT  ) {
				out += ",\"x\":";
				add_int(out, c.x);
				out += ",\"y\":";
				add_int(out, c.y);
				if(  kind != live_changes_t::K_CITY  ) {
					out += ",\"z\":";
					add_int(out, c.z);
				}
			}
			else {
				out += kind == live_changes_t::K_PLAYER ? ",\"nr\":" : ",\"id\":";
				add_int(out, c.id);
			}
			out += ",\"what\":[";
			bool first = true;
			for(  int bit = 0;  bit < live_changes_t::W_MAX_BIT;  bit++  ) {
				if(  c.value & (1 << bit)  ) {
					if(  !first  ) {
						out += ',';
					}
					first = false;
					out += '"';
					out += WHAT_NAMES[bit];
					out += '"';
				}
			}
			out += "]}";
			break;
		}

		case CHANGE_SAVED: {
			out += "{\"type\":\"saved\",\"sync\":";
			add_int(out, c.sync);
			out += ",\"ticks\":";
			add_int(out, c.value);
			out += ",\"file\":";
			auto it = saved_files.find(seq);
			add_str(out, it != saved_files.end() ? it->second.c_str() : "");
			out += '}';
			break;
		}

		case CHANGE_MONTH:
			out += "{\"type\":\"month\",\"sync\":";
			add_int(out, c.sync);
			out += ",\"ticks\":";
			add_int(out, c.value);
			out += ",\"month\":";
			add_int(out, c.id);
			out += '}';
			break;
	}
}

}


namespace {

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


void add_air_indices(std::string &out, vehicle_t *v)
{
	if(  v->get_waytype() != air_wt  ) {
		out += "null";
		return;
	}
	air_vehicle_t::flight_state state;
	uint32 takeoff, search, touchdown;
	static_cast<air_vehicle_t *>(v)->get_event_index(state, takeoff, search, touchdown);
	out += '[';
	add_int(out, takeoff);
	out += ',';
	add_int(out, search);
	out += ',';
	add_int(out, touchdown);
	out += ']';
}

/**
 * The direction of the image the vehicle shows: the one of the images of its type that is
 * the image now (vehicle_t::calc_image takes vehicle_t::get_image_direction, a road vehicle
 * leaving a stop sideways the side of the direction, road_vehicle_t::set_sideways_image).
 */
ribi_t::ribi image_direction(const vehicle_t *v)
{
	// while loading, the convoy is (convoi_t *)1
	const convoi_t *cnv = v->get_convoi() == (convoi_t *)1 ? NULL : v->get_convoi();
	const bool reversed = cnv ? cnv->is_reversed() : false;
	// vehicle_t::get_image_direction
	const ribi_t::ribi dir = reversed ? ribi_t::backward(v->get_direction()) : v->get_direction();
	const vehicle_desc_t *desc = v->get_desc();
	const image_id image = v->get_image();
	if(  desc == NULL  ||  image == IMG_EMPTY  ) {
		return dir;
	}
	const bool no_electric = cnv ? !cnv->get_use_electric() : false;
	const goods_desc_t *goods = v->get_cargo().empty() ? NULL : v->get_cargo().front().get_desc();
	if(  desc->get_image_id(ribi_t::get_dir(dir), goods, reversed, no_electric) == image  ) {
		return dir;
	}
	static const ribi_t::ribi dirs[8] = {
		ribi_t::south, ribi_t::west, ribi_t::southwest, ribi_t::southeast,
		ribi_t::north, ribi_t::east, ribi_t::northeast, ribi_t::northwest
	};
	for(  int i = 0;  i < 8;  i++  ) {
		if(  desc->get_image_id(ribi_t::get_dir(dirs[i]), goods, reversed, no_electric) == image  ) {
			return dirs[i];
		}
	}
	return dir;
}


// ---------------------------------------------------------------------------
// get_live_state
// ---------------------------------------------------------------------------

enum vehicle_field_t {
	VF_X, VF_Y, VF_Z, VF_DIR, VF_STEPS, VF_STEPS_NEXT, VF_RI, VF_LOAD, VF_XOFF, VF_YOFF, VF_AIR,
	VF_NX, VF_NY, VF_NZ, VF_PREV_DIR, VF_DX, VF_DY, VF_HOFF, VF_ZOFF_START, VF_ZOFF_END,
	VF_LANE, VF_NEXT_LANE, VF_SPEED_LIMIT, VF_ROUTE_INDEX, VF_PX, VF_PY, VF_PZ,
	VF_LEADING, VF_LAST, VF_CARGO, VF_CAPACITY, VF_OVERTAKING, VF_WITHDRAW, VF_IN_DEPOT,
	VF_DIST, VF_VID, VF_VSN, VF_IDIR, VF_AIR_IX
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
	{ "in_depot", VF_IN_DEPOT },
	// position and identity
	{ "dist", VF_DIST },                // steps from the start of the route (route_tile_lengths), -1 unknown
	{ "vid", VF_VID },                  // vehicle type: index in "vnames"
	{ "vsn", VF_VSN },                  // place in the convoy as assembled (does not change when the convoy reverses)
	// the picture
	{ "idir", VF_IDIR },                // direction (ribi) of the image shown
	{ "air_ix", VF_AIR_IX }             // aircraft: [takeoff, search for stop, touchdown] (route indices as saved), null if no aircraft
};
const int NUM_VEHICLE_FIELDS = sizeof(VEHICLE_FIELDS) / sizeof(VEHICLE_FIELDS[0]);
const int NUM_DEFAULT_FIELDS = VF_AIR + 1;

/**
 * Length in steps of every tile of a route, as the vehicles drive it: a
 * vehicle entering tile i heads from tile i-1 to tile i+1
 * (vehicle_t::hop -> vehicle_base_t::calc_set_direction): straight across
 * the tile it takes VEHICLE_STEPS_PER_TILE steps, diagonally (a curve)
 * diagonal_vehicle_steps_per_tile. The first and the last tile are straight
 * (vehicle_t::initialise_journey; the end of the route lies straight ahead),
 * and so is a tile where the route turns back on itself (measured: road
 * vehicles take a whole tile to turn). Not covered: the shortened last tile
 * of the leading vehicle at a stop and at a coupling point.
 */
void route_tile_lengths(const route_t *route, std::vector<uint32> &len)
{
	const uint32 n = route->get_count();
	const uint32 diagonal = vehicle_base_t::get_diagonal_vehicle_steps_per_tile();
	len.resize(n);
	for(  uint32 i = 0;  i < n;  i++  ) {
		if(  i == 0  ||  i + 1 >= n  ) {
			len[i] = VEHICLE_STEPS_PER_TILE;
			continue;
		}
		const koord3d &prev = route->at((uint16)(i - 1));
		const koord3d &next = route->at((uint16)(i + 1));
		if(  prev == next  ) {
			len[i] = VEHICLE_STEPS_PER_TILE;
			continue;
		}
		const sint32 di = next.x - prev.x;
		const sint32 dj = next.y - prev.y;
		const bool straight = (di == 0  &&  dj != 0)  ||  (dj == 0  &&  di != 0);
		len[i] = straight ? VEHICLE_STEPS_PER_TILE : diagonal;
	}
}

/// what the vehicle fields of one convoy need
struct convoy_context_t
{
	const convoi_t *cnv;
	const route_t *route;
	std::vector<uint32> dist_before;   ///< steps from the start of the route to the start of tile i (only if "dist" is asked for)
	uint8 count;
	bool reversed;
};

void add_vehicle_field(std::string &out, vehicle_field_t f, const convoy_context_t &ctx, uint8 index, vehicle_t *v)
{
	switch(  f  ) {
		case VF_X:            add_int(out, v->get_pos().x); break;
		case VF_Y:            add_int(out, v->get_pos().y); break;
		case VF_Z:            add_int(out, v->get_pos().z); break;
		case VF_DIR:          add_int(out, v->get_direction()); break;
		case VF_STEPS:        add_int(out, v->get_steps()); break;
		case VF_STEPS_NEXT:   add_int(out, v->get_steps_next()); break;
		case VF_RI:           add_int(out, route_index_of_own_tile(v, ctx.route)); break;
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
		case VF_OVERTAKING:   add_int(out, ctx.cnv->get_tiles_overtaking()); break;
		case VF_WITHDRAW:     add_int(out, ctx.cnv->get_withdraw()); break;
		case VF_IN_DEPOT:     add_int(out, ctx.cnv->in_depot()); break;
		case VF_DIST: {
			const sint32 ri = route_index_of_own_tile(v, ctx.route);
			add_int(out, ri < 0 ? -1 : (sint64)ctx.dist_before[ri] + v->get_steps());
			break;
		}
		case VF_VID:          add_int(out, live_util::vehicle_name_index(v->get_desc())); break;
		case VF_VSN:          add_int(out, ctx.reversed ? ctx.count - 1 - index : index); break;
		case VF_IDIR:         add_int(out, image_direction(v)); break;
		case VF_AIR_IX:       add_air_indices(out, v); break;
	}
}

/// route laps of every convoy (see live_changes_t::route_restarted): convoy id -> laps, and the route and position last seen
struct route_lap_t
{
	uint32 laps;
	uint32 hash;
	sint32 front_ri;   ///< route index of the tile of the front vehicle
	bool hash_known;
};
std::unordered_map<uint32, route_lap_t> route_laps;

} // namespace


/// fields of the start of every answer: the world and its time
void live_state::add_head(std::string &out, karte_t *welt)
{
	static const uint8 days_per_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	out += "\"epoch\":";
	add_int(out, live_changes_t::get_epoch());
	out += ",\"sync\":";
	add_int(out, welt->get_sync_steps());

	// [year, month 1-12, day 1-31], as the date display counts days
	const uint32 month = welt->get_current_month();
	const uint32 tpm = welt->ticks_per_world_month;
	const uint32 ticks_this_month = welt->get_ticks() % tpm;
	out += ",\"date\":[";
	add_int(out, month / 12);
	out += ',';
	add_int(out, month % 12 + 1);
	out += ',';
	add_int(out, 1 + (sint64)ticks_this_month * days_per_month[month % 12] / tpm);
	out += "],\"ticks\":";
	add_int(out, welt->get_ticks());
	out += ",\"ticks_per_month\":";
	add_int(out, tpm);

	// real time of a sync step in ms (karte_t::fix_ratio_frame_time in network games), and the game speed
	sint64 pace;
	if(  env_t::networkmode  ) {
		pace = 1000 / clamp(welt->get_settings().get_frames_per_second(), env_t::min_fps, env_t::max_fps);
	}
	else {
		pace = 1000 / (welt->is_fast_forward() ? env_t::ff_fps : env_t::fps);
	}
	out += ",\"pace\":";
	add_int(out, pace);
	out += ",\"time_multiplier\":";
	add_int(out, welt->get_time_multiplier());

	// in network games every sync step advances ticks by the same amount (karte_t::interactive:
	// sync_step(fix_ratio_frame_time * time_multiplier / 16)), so ticks = ticks_per_sync * sync + ticks_base
	// as long as the game speed does not change; null in single player games
	out += ",\"ticks_per_sync\":";
	if(  env_t::networkmode  ) {
		const sint64 per_sync = pace * welt->get_time_multiplier() / 16;
		add_int(out, per_sync);
		out += ",\"ticks_base\":";
		add_int(out, (sint64)welt->get_ticks() - per_sync * (sint64)welt->get_sync_steps());
	}
	else {
		out += "null,\"ticks_base\":null";
	}
	out += ",\"snowline\":";
	add_int(out, welt->get_snowline());
}


void live_changes_t::route_restarted_intern(uint32 convoy)
{
	route_laps[convoy].laps++;
}


void live_changes_t::forget_world()
{
	route_laps.clear();
	live_util::clear_all_changes();
}


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
	bool want_dist = false;
	bool need_vnames = false;
	for(  int f : fields  ) {
		want_dist |= VEHICLE_FIELDS[f].field == VF_DIST;
		need_vnames |= VEHICLE_FIELDS[f].field == VF_VID;
	}

	// routes are most of the answer: they can be left out (rt null), sent for some convoys only, and replaced by hashes
	const bool all_routes = mcp_server_t::json_get_raw(args_json, "routes") != "false";
	const std::string route_ids_raw = mcp_server_t::json_get_raw(args_json, "route_ids");
	const bool some_routes = !route_ids_raw.empty()  &&  route_ids_raw != "null";
	std::vector<sint64> route_ids;
	if(  some_routes  ) {
		route_ids = parse_int_array(route_ids_raw);
		std::sort(route_ids.begin(), route_ids.end());
	}
	const bool with_route_hash = get_bool_arg(args_json, "route_hash");
	const bool with_tile_lens = get_bool_arg(args_json, "tile_lens");
	const bool with_stops = get_bool_arg(args_json, "stops");

	// only what changed since the caller's previous answer; the convoys are compared only when since is given
	// (null at first, as for get_changes): hashing all records costs about as much as building them
	const bool track = !mcp_server_t::json_get_raw(args_json, "since").empty();
	sint64 since_arg = 0;
	const bool has_since = get_int_arg(args_json, "since", since_arg)  &&  since_arg >= 0  &&  since_arg <= 0xFFFFFFFFll;
	const uint32 since = has_since ? (uint32)since_arg : 0;
	// the records of the convoys depend on what was asked for: one change detection for each kind of request
	std::string shape = "c";
	for(  int f : fields  ) {
		shape += ',';
		shape += VEHICLE_FIELDS[f].name;
	}
	shape += all_routes ? "|r" : "|-";
	shape += route_ids_raw;
	shape += with_tile_lens ? "|t" : "";
	shape += with_stops ? "|s" : "";
	live_util::entity_changes_t &convoy_changes = live_util::changes_of(shape);
	if(  track  ) {
		convoy_changes.begin(has_since, since);
	}

	// frames per second of sync_steps (fixed in network games)
	sint64 fps = env_t::fps;
	if(  env_t::networkmode  ) {
		fps = 1000 / (1000 / clamp(welt->get_settings().get_frames_per_second(), env_t::min_fps, env_t::max_fps));
	}

	std::string out;
	out.reserve(1 << 20);
	out += "{\"schema\":1,";
	live_state::add_head(out, welt);
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
	out += "],\"cf\":[\"id\",\"owner\",\"state\",\"speed\",\"reversed\",\"rt0\",\"rt\",\"v\",\"rlap\"";
	if(  with_tile_lens  ) {
		out += ",\"tl\"";
	}
	if(  with_stops  ) {
		out += ",\"cx\"";
	}
	out += "],\"c\":[";

	// [id, owner, state, speed (km/h), reversed, rt0, rt, vehicles, rlap (, tl) (, cx)]
	bool first_cnv = true;
	std::string rec;
	std::string route_hashes;
	std::vector<uint32> lengths;
	convoy_context_t ctx;
	for(  const convoihandle_t &cnv : welt->convoys()  ) {
		if(  !cnv.is_bound()  ) {
			continue;
		}
		const route_t *route = cnv->get_route();
		const bool with_route = some_routes ? std::binary_search(route_ids.begin(), route_ids.end(), (sint64)cnv.get_id()) : all_routes;

		// a new lap: the route was driven again from its start (route_restarted), it changed, or the front
		// vehicle went back on it (vehicle_t::reanchor_route_index finds a tile that a route visits twice)
		const uint32 hash = route_hash(route);
		const sint32 front_ri = cnv->get_vehicle_count() > 0 ? route_index_of_own_tile(cnv->front(), route) : -1;
		route_lap_t &lap = route_laps[cnv.get_id()];
		if(  lap.hash_known  &&  (lap.hash != hash  ||  front_ri < lap.front_ri)  ) {
			lap.laps++;
		}
		lap.hash = hash;
		lap.front_ri = front_ri;
		lap.hash_known = true;

		ctx.cnv = cnv.get_rep();
		ctx.route = route;
		ctx.count = cnv->get_vehicle_count();
		ctx.reversed = cnv->is_reversed();
		if(  want_dist  ||  (with_tile_lens  &&  with_route)  ) {
			route_tile_lengths(route, lengths);
			ctx.dist_before.resize(lengths.size() + 1);
			ctx.dist_before[0] = 0;
			for(  size_t i = 0;  i < lengths.size();  i++  ) {
				ctx.dist_before[i + 1] = ctx.dist_before[i] + lengths[i];
			}
		}

		rec = "[";
		add_int(rec, cnv.get_id());
		rec += ',';
		add_int(rec, cnv->get_owner() ? cnv->get_owner()->get_player_nr() : -1);
		rec += ',';
		add_int(rec, cnv->get_state());
		rec += ',';
		add_double(rec, cnv->get_akt_speed() * (double)VEHICLE_SPEED_FACTOR / 64.0, 1);
		rec += ',';
		add_int(rec, cnv->is_reversed());
		// the whole route, so rt0 is always 0
		rec += ",0,";
		if(  with_route  ) {
			rec += '[';
			for(  uint32 i = 0;  i < route->get_count();  i++  ) {
				const koord3d &k = route->at((uint16)i);
				if(  i > 0  ) {
					rec += ',';
				}
				add_int(rec, k.x);
				rec += ',';
				add_int(rec, k.y);
				rec += ',';
				add_int(rec, k.z);
			}
			rec += ']';
		}
		else {
			rec += "null";
		}
		rec += ",[";
		// in the order of the savegame (front() first)
		for(  uint8 i = 0;  i < ctx.count;  i++  ) {
			vehicle_t *v = cnv->get_vehikel(i);
			if(  i > 0  ) {
				rec += ',';
			}
			rec += '[';
			for(  size_t f = 0;  f < fields.size();  f++  ) {
				if(  f > 0  ) {
					rec += ',';
				}
				add_vehicle_field(rec, VEHICLE_FIELDS[fields[f]].field, ctx, i, v);
			}
			rec += ']';
		}
		rec += "],";
		add_int(rec, lap.laps);
		if(  with_tile_lens  ) {
			rec += ',';
			if(  with_route  ) {
				rec += '[';
				for(  size_t i = 0;  i < lengths.size();  i++  ) {
					if(  i > 0  ) {
						rec += ',';
					}
					add_int(rec, lengths[i]);
				}
				rec += ']';
			}
			else {
				rec += "null";
			}
		}
		if(  with_stops  ) {
			// [current stop, arrived time, departure time (0 none), tiles since the last stop, sum of their speed limits, route length]
			const schedule_t *schedule = cnv->get_schedule();
			rec += ",[";
			add_int(rec, schedule ? schedule->get_current_stop() : -1);
			rec += ',';
			add_int(rec, cnv->get_arrived_time());
			rec += ',';
			add_int(rec, cnv->get_departure_time());
			rec += ',';
			add_int(rec, cnv->get_distance_since_last_stop());
			rec += ',';
			add_int(rec, cnv->get_sum_speed_limit());
			rec += ',';
			add_int(rec, route->get_count());
			rec += ']';
		}
		rec += ']';

		if(  !track  ||  convoy_changes.update(cnv.get_id(), rec)  ) {
			if(  !first_cnv  ) {
				out += ',';
			}
			first_cnv = false;
			out += rec;
			if(  with_route_hash  ) {
				if(  !route_hashes.empty()  ) {
					route_hashes += ',';
				}
				add_int(route_hashes, hash);
			}
		}
	}
	out += "],\"gone\":[";
	if(  track  ) {
		bool first = true;
		for(  uint64 id : convoy_changes.end()  ) {
			if(  !first  ) {
				out += ',';
			}
			first = false;
			add_int(out, (sint64)id);
		}
	}
	out += "],\"full\":";
	out += !track  ||  convoy_changes.is_full() ? "true" : "false";

	if(  with_route_hash  ) {
		out += ",\"rh\":[" + route_hashes + "]";
	}

	live_util::add_world_sections(out, welt, args_json, has_since, since, need_vnames);

	if(  need_vnames  ) {
		out += ',';
		live_util::add_vehicle_names(out);
	}

	out += ",\"seq\":";
	add_int(out, live_util::entity_changes_t::get_seq());
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
	live_state::add_head(out, welt);
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
	file.close();
	out += ",\"th\":";
	add_int(out, live_util::fnv1a(data));
	out += ",\"data\":\"";
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
	live_state::add_head(out, welt);
	out += ",\"seq\":";
	add_int(out, next_seq);
	out += ",\"floor\":";
	add_int(out, first_seq);
	out += ",\"overflow\":";
	out += overflow ? "true" : "false";

	std::string tiles, sig, events;
	if(  has_since  &&  !overflow  ) {
		for(  uint32 seq = since;  seq != next_seq;  seq++  ) {
			const change_t &c = change_log[seq % LOG_SIZE];
			if(  c.typ >= CHANGE_MONTH  ) {
				if(  !events.empty()  ) {
					events += ',';
				}
				add_event(events, c, seq);
				continue;
			}
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
			if(  c.typ == 0  ) {
				// [sync, x, y, kind bits]
				list += ',';
				add_int(list, c.id ? c.id : (uint32)T_OTHER);
			}
			else {
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
	out += ",\"tiles\":[" + tiles + "],\"sig\":[" + sig + "],\"events\":[" + events + "]}";
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
// get_messages
// ---------------------------------------------------------------------------

std::string live_state::get_messages(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	sint64 since = 0;
	const bool has_since = get_int_arg(args_json, "since", since);
	if(  has_since  &&  (since < 0  ||  since > 0xFFFFFFFFll)  ) {
		return error_json("bad since");
	}
	const bool overflow = has_since  &&  ((uint32)since < message_first_seq  ||  (uint32)since > message_next_seq);

	std::string out = "{";
	live_state::add_head(out, welt);
	out += ",\"seq\":";
	add_int(out, message_next_seq);
	out += ",\"floor\":";
	add_int(out, message_first_seq);
	out += ",\"overflow\":";
	out += overflow ? "true" : "false";
	out += ",\"messages\":[";
	// without since: everything still kept
	const uint32 from = !has_since  ||  overflow ? message_first_seq : (uint32)since;
	for(  uint32 seq = from;  seq != message_next_seq;  seq++  ) {
		const message_entry_t &m = message_log[seq % MESSAGE_LOG_SIZE];
		if(  seq != from  ) {
			out += ',';
		}
		out += "{\"seq\":";
		add_int(out, m.seq);
		out += ",\"sync\":";
		add_int(out, m.sync);
		out += ",\"month\":";
		add_int(out, m.month);
		out += ",\"type\":";
		add_int(out, m.type);
		out += ",\"text\":";
		add_str(out, m.text.c_str());
		out += ",\"player\":";
		add_int(out, m.player);
		out += ",\"x\":";
		add_int(out, m.x);
		out += ",\"y\":";
		add_int(out, m.y);
		out += '}';
	}
	out += "]}";
	return out;
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
	live_state::add_head(out, welt);
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
