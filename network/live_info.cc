/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * The MCP tools get_info (the basic information of stations, convoys, lines,
 * factories, cities, players and depots) and get_history (their statistics
 * as the savegame stores them). Both are read-only and run on the main thread.
 *
 * Every record of get_info ends with a hash of the record (FNV-1a over its
 * JSON text): with hash_only the caller gets [key, hash] only, and asks
 * again with ids for the records whose hash changed.
 */

#include "live_state.h"
#include "live_util.h"
#include "mcp_server.h"

#include <algorithm>
#include <functional>
#include <set>

#include "../simworld.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simfab.h"
#include "../simcity.h"
#include "../simdepot.h"
#include "../simline.h"
#include "../simplan.h"
#include "../boden/grund.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../player/finance.h"
#include "../dataobj/schedule.h"
#include "../descriptor/factory_desc.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/vehicle_desc.h"

using namespace live_util;


namespace {

enum info_level_t { LEVEL_BASIC, LEVEL_FULL, LEVEL_AREA };

void add_pos(std::string &out, koord3d k)
{
	out += '[';
	add_int(out, k.x);
	out += ',';
	add_int(out, k.y);
	out += ',';
	add_int(out, k.z);
	out += ']';
}

sint64 line_id_or_none(linehandle_t line)
{
	return line.is_bound() ? (sint64)line.get_id() : -1;
}

int player_nr_of(const player_t *player)
{
	return player ? player->get_player_nr() : -1;
}

/**
 * A schedule: [current, flags, max_speed, base waiting time, next line, [entries]], each entry
 * [x, y, z, stop flags, minimum loading, waiting time shift]. The full level adds the departure slot
 * group and the waytype before the entries, and to each entry: maximum loading, length coupling done,
 * max speed, balance speed, spacing, spacing shift, delay tolerance, [journey times], [waiting times],
 * [stopping times] (the last 5 each, newest first). The entries are those of the savegame (with the dummy
 * entry that a next line adds).
 * The current stop is written into `current` instead of `out`: it is left out of the record's hash.
 */
void add_schedule(std::string &out, std::string &rest, const schedule_t *schedule, info_level_t level)
{
	if(  !schedule  ) {
		out += "null";
		return;
	}
	out += '[';
	add_int(out, schedule->get_current_stop());
	// what follows the current stop goes into the hash
	rest += ',';
	add_int(rest, schedule->get_flags());
	rest += ',';
	add_int(rest, schedule->get_max_speed());
	rest += ',';
	add_int(rest, schedule->get_additional_base_waiting_time());
	rest += ',';
	add_int(rest, line_id_or_none(schedule->get_next_line()));
	if(  level == LEVEL_FULL  ) {
		rest += ',';
		add_int(rest, line_id_or_none(schedule->get_departure_slot_group_id()));
		rest += ',';
		add_int(rest, schedule->get_waytype());
	}
	rest += ",[";
	bool first = true;
	for(  const schedule_entry_t &e : schedule->get_entries()  ) {
		if(  !first  ) {
			rest += ',';
		}
		first = false;
		rest += '[';
		add_int(rest, e.pos.x);
		rest += ',';
		add_int(rest, e.pos.y);
		rest += ',';
		add_int(rest, e.pos.z);
		rest += ',';
		add_int(rest, e.get_stop_flags());
		rest += ',';
		add_int(rest, e.minimum_loading);
		rest += ',';
		add_int(rest, e.waiting_time_shift);
		if(  level == LEVEL_FULL  ) {
			const uint16 values[7] = { e.maximum_loading, e.length_coupling_done, e.max_speed_kmh_of_convoi, e.balance_speed_kmh_of_convoi, e.spacing, e.spacing_shift, e.delay_tolerance };
			for(  uint16 v : values  ) {
				rest += ',';
				add_int(rest, v);
			}
			// newest first, as the script API gives them
			const auto add_times = [&](const uint32 *times, int count, uint8 next) {
				rest += ",[";
				for(  int i = 0;  i < count;  i++  ) {
					if(  i > 0  ) {
						rest += ',';
					}
					add_int(rest, times[(next - 1 - i + count) % count]);
				}
				rest += ']';
			};
			add_times(e.journey_time, NUM_ARRIVAL_TIME_STORED, e.jt_at_index);
			add_times(e.waiting_time, NUM_WAITING_TIME_STORED, e.wt_at_index);
			add_times(e.convoy_stopping_time, NUM_STOPPING_TIME_STORED, e.cs_at_index);
		}
		rest += ']';
	}
	rest += "]]";
}

/**
 * Collects the records of one kind and writes them as rows.
 * A record is built as `head` (hashed), then optionally `unhashed` (not hashed, e.g. the current
 * stop), then `tail` (hashed); the row is head + unhashed + tail + "," + hash + "]".
 * The hash is always the one of the basic record (hash_tail: the tail of the basic level, if
 * the record is of the full level), so it does not depend on the level asked for.
 */
struct rows_t
{
	std::string out;
	bool hash_only;
	bool first = true;

	explicit rows_t(bool hash_only) : hash_only(hash_only) {}

	/// @param key the key of the record as JSON values without brackets ("12" or "3,4,5")
	void add(const std::string &key, const std::string &head, const std::string &unhashed, const std::string &tail, const std::string *hash_tail = NULL)
	{
		const uint32 h = fnv1a(hash_tail ? *hash_tail : tail, fnv1a(head));
		if(  !first  ) {
			out += ',';
		}
		first = false;
		if(  hash_only  ) {
			out += '[';
			out += key;
			out += ',';
		}
		else {
			out += head;
			out += unhashed;
			out += tail;
			out += ',';
		}
		add_int(out, h);
		out += ']';
	}
};

/// the ids asked for (empty: all), and the ones found
struct id_filter_t
{
	std::vector<std::vector<sint64>> keys;
	std::vector<bool> found;
	bool all;

	explicit id_filter_t(const std::string &raw) : all(raw.empty()  ||  raw == "null")
	{
		if(  !all  ) {
			keys = parse_int_groups(raw);
			found.resize(keys.size(), false);
		}
	}

	/// for an entity with these key values (a position matches without z, if z was not given)
	bool wants(sint64 a, sint64 b = 0, sint64 c = 0, int n = 1)
	{
		if(  all  ) {
			return true;
		}
		bool any = false;
		for(  size_t i = 0;  i < keys.size();  i++  ) {
			const std::vector<sint64> &k = keys[i];
			if(  k.empty()  ||  k[0] != a  ) {
				continue;
			}
			if(  n >= 2  &&  (k.size() < 2  ||  k[1] != b)  ) {
				continue;
			}
			if(  n >= 3  &&  k.size() >= 3  &&  k[2] != c  ) {
				continue;
			}
			found[i] = true;
			any = true;
		}
		return any;
	}

	/// the keys asked for and not found, as a JSON array
	std::string gone() const
	{
		std::string out = "[";
		bool first = true;
		for(  size_t i = 0;  i < keys.size();  i++  ) {
			if(  found[i]  ) {
				continue;
			}
			if(  !first  ) {
				out += ',';
			}
			first = false;
			if(  keys[i].size() == 1  ) {
				add_int(out, keys[i][0]);
			}
			else {
				out += '[';
				for(  size_t j = 0;  j < keys[i].size();  j++  ) {
					if(  j > 0  ) {
						out += ',';
					}
					add_int(out, keys[i][j]);
				}
				out += ']';
			}
		}
		return out + "]";
	}
};

std::string key_of(sint64 a)
{
	std::string k;
	add_int(k, a);
	return k;
}

std::string key_of(koord3d pos, bool with_z = true)
{
	std::string k;
	add_int(k, pos.x);
	k += ',';
	add_int(k, pos.y);
	if(  with_z  ) {
		k += ',';
		add_int(k, pos.z);
	}
	return k;
}

/// the tiles a halt serves (planquadrat_t::is_connected), as rows of [y, x0, x1]
void add_halt_area(std::string &out, karte_t *welt, halthandle_t halt)
{
	const sint16 cov = welt->get_settings().get_station_coverage();
	koord lo(0x7FFF, 0x7FFF), hi(-1, -1);
	for(  const haltestelle_t::tile_t &t : halt->get_tiles()  ) {
		const koord k = t.grund->get_pos().get_2d();
		lo.x = min(lo.x, k.x - cov);
		lo.y = min(lo.y, k.y - cov);
		hi.x = max(hi.x, k.x + cov);
		hi.y = max(hi.y, k.y + cov);
	}
	lo.x = max(lo.x, (sint16)0);
	lo.y = max(lo.y, (sint16)0);
	hi.x = min(hi.x, (sint16)(welt->get_size().x - 1));
	hi.y = min(hi.y, (sint16)(welt->get_size().y - 1));
	out += '[';
	bool first = true;
	for(  sint16 y = lo.y;  y <= hi.y;  y++  ) {
		sint16 start = -1;
		for(  sint16 x = lo.x;  x <= hi.x + 1;  x++  ) {
			const bool in = x <= hi.x  &&  welt->access(x, y)->is_connected(halt);
			if(  in  &&  start < 0  ) {
				start = x;
			}
			else if(  !in  &&  start >= 0  ) {
				if(  !first  ) {
					out += ',';
				}
				first = false;
				out += '[';
				add_int(out, y);
				out += ',';
				add_int(out, start);
				out += ',';
				add_int(out, x - 1);
				out += ']';
				start = -1;
			}
		}
	}
	out += ']';
}

const char *const KINDS[] = { "halts", "convoys", "lines", "factories", "cities", "players", "depots" };

void info_halts(rows_t &rows, id_filter_t &ids, karte_t *welt, info_level_t level)
{
	std::string head, tail;
	for(  const halthandle_t &halt : haltestelle_t::get_alle_haltestellen()  ) {
		if(  !halt.is_bound()  ||  !ids.wants(halt.get_id())  ) {
			continue;
		}
		head = "[";
		add_int(head, halt.get_id());
		head += ',';
		if(  level == LEVEL_AREA  ) {
			// [id, [[y, x0, x1], ...], hash]
			add_halt_area(head, welt, halt);
			rows.add(key_of(halt.get_id()), head, "", "");
			continue;
		}
		// [id, name, owner, [[x, y, z], ...], station type bits, enabled bits (1 passengers, 2 mail, 4 goods), hash]
		add_str(head, halt->get_name());
		head += ',';
		add_int(head, player_nr_of(halt->get_owner()));
		head += ",[";
		bool first = true;
		for(  const haltestelle_t::tile_t &t : halt->get_tiles()  ) {
			if(  !first  ) {
				head += ',';
			}
			first = false;
			add_pos(head, t.grund->get_pos());
		}
		head += "],";
		add_int(head, halt->get_station_type());
		head += ',';
		add_int(head, (halt->get_pax_enabled() ? 1 : 0) | (halt->get_mail_enabled() ? 2 : 0) | (halt->get_ware_enabled() ? 4 : 0));
		rows.add(key_of(halt.get_id()), head, "", "");
	}
}

void info_convoys(rows_t &rows, id_filter_t &ids, karte_t *welt, info_level_t level)
{
	std::string head, current, tail, basic_current, basic_tail;
	for(  const convoihandle_t &cnv : welt->convoys()  ) {
		if(  !cnv.is_bound()  ||  !ids.wants(cnv.get_id())  ) {
			continue;
		}
		// [id, name, owner, line (-1 none), state, home depot [x, y, z], [vehicle types (vnames index)], schedule, vtype, hash]
		head = "[";
		add_int(head, cnv.get_id());
		head += ',';
		add_str(head, cnv->get_name());
		head += ',';
		add_int(head, player_nr_of(cnv->get_owner()));
		head += ',';
		add_int(head, line_id_or_none(cnv->get_line()));
		head += ',';
		add_int(head, cnv->get_state());
		head += ',';
		add_pos(head, cnv->get_home_depot());
		head += ",[";
		for(  uint8 i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			if(  i > 0  ) {
				head += ',';
			}
			add_int(head, vehicle_name_index(cnv->get_vehikel(i)->get_desc()));
		}
		head += "],";
		current.clear();
		tail.clear();
		add_schedule(current, tail, cnv->get_schedule(), level);
		if(  level == LEVEL_FULL  ) {
			basic_current.clear();
			basic_tail.clear();
			add_schedule(basic_current, basic_tail, cnv->get_schedule(), LEVEL_BASIC);
		}
		// the obj_t::typ of the front vehicle (road_vehicle, rail_vehicle, ...), -1 without vehicles
		std::string vtype = ",";
		add_int(vtype, cnv->get_vehicle_count() > 0 ? (sint64)cnv->front()->get_typ() : -1);
		tail += vtype;
		basic_tail += vtype;
		rows.add(key_of(cnv.get_id()), head, current, tail, level == LEVEL_FULL ? &basic_tail : NULL);
	}
}

void info_lines(rows_t &rows, id_filter_t &ids, karte_t *welt, info_level_t level)
{
	std::string head, current, tail, basic_current, basic_tail;
	vector_tpl<linehandle_t> lines;
	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		player_t *player = welt->get_player(i);
		if(  !player  ) {
			continue;
		}
		lines.clear();
		player->simlinemgmt.get_lines(simline_t::line, &lines);
		for(  const linehandle_t &line : lines  ) {
			if(  !ids.wants(line.get_id())  ) {
				continue;
			}
			// [id, name, owner, line type, colour (palette index), schedule, [convoy ids], hash]
			head = "[";
			add_int(head, line.get_id());
			head += ',';
			add_str(head, line->get_name());
			head += ',';
			add_int(head, i);
			head += ',';
			add_int(head, line->get_linetype());
			head += ',';
			add_int(head, line->get_colour());
			head += ',';
			current.clear();
			tail.clear();
			add_schedule(current, tail, line->get_schedule(), level);
			std::string convoys = ",[";
			bool first = true;
			for(  const convoihandle_t &cnv : line->get_convoys()  ) {
				if(  !first  ) {
					convoys += ',';
				}
				first = false;
				add_int(convoys, cnv.get_id());
			}
			convoys += ']';
			tail += convoys;
			if(  level == LEVEL_FULL  ) {
				basic_current.clear();
				basic_tail.clear();
				add_schedule(basic_current, basic_tail, line->get_schedule(), LEVEL_BASIC);
				basic_tail += convoys;
			}
			rows.add(key_of(line.get_id()), head, current, tail, level == LEVEL_FULL ? &basic_tail : NULL);
		}
	}
}

void add_goods_names(std::string &out, const array_tpl<ware_production_t> &list)
{
	out += '[';
	for(  uint32 i = 0;  i < list.get_count();  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		add_str(out, list[i].get_typ()->get_name());
	}
	out += ']';
}

void info_factories(rows_t &rows, id_filter_t &ids, karte_t *welt)
{
	std::string head;
	for(  fabrik_t *fab : welt->get_fab_list()  ) {
		const koord3d pos = fab->get_pos();
		if(  !ids.wants(pos.x, pos.y, pos.z, 3)  ) {
			continue;
		}
		// [x, y, z, name (pak), name given by a player ("" none), owner, base production, [[field x, y, class], ...], [input goods], [output goods], hash]
		head = "[";
		head += key_of(pos);
		head += ',';
		add_str(head, fab->get_desc()->get_name());
		head += ',';
		add_str(head, fab->get_custom_name());
		head += ',';
		add_int(head, player_nr_of(fab->get_owner()));
		head += ',';
		add_int(head, fab->get_base_production());
		head += ",[";
		for(  uint32 i = 0;  i < fab->get_field_count();  i++  ) {
			if(  i > 0  ) {
				head += ',';
			}
			const koord k = fab->get_field_pos(i);
			head += '[';
			add_int(head, k.x);
			head += ',';
			add_int(head, k.y);
			head += ',';
			add_int(head, fab->get_field_class(i));
			head += ']';
		}
		head += "],";
		add_goods_names(head, fab->get_input());
		head += ',';
		add_goods_names(head, fab->get_output());
		rows.add(key_of(pos), head, "", "");
	}
}

void info_cities(rows_t &rows, id_filter_t &ids, karte_t *welt)
{
	std::string head;
	for(  stadt_t *city : welt->get_cities()  ) {
		const koord pos = city->get_pos();
		if(  !ids.wants(pos.x, pos.y, 0, 2)  ) {
			continue;
		}
		// [x, y, name, population, [lo x, lo y, ur x, ur y], citizens (bev), jobs (arb), homes (won), hash]
		head = "[";
		head += key_of(koord3d(pos, 0), false);
		head += ',';
		add_str(head, city->get_name());
		head += ',';
		add_int(head, city->get_einwohner());
		head += ",[";
		add_int(head, city->get_linksoben().x);
		head += ',';
		add_int(head, city->get_linksoben().y);
		head += ',';
		add_int(head, city->get_rechtsunten().x);
		head += ',';
		add_int(head, city->get_rechtsunten().y);
		head += "],";
		add_int(head, city->get_bev());
		head += ',';
		add_int(head, city->get_arb());
		head += ',';
		add_int(head, city->get_won());
		rows.add(key_of(koord3d(pos, 0), false), head, "", "");
	}
}

void info_players(rows_t &rows, id_filter_t &ids, karte_t *welt)
{
	std::string head;
	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		const player_t *player = welt->get_player(i);
		if(  !player  ||  !ids.wants(i)  ) {
			continue;
		}
		// [nr, name, colour 1, colour 2, type (ai id, 0 human), locked, hash]
		head = "[";
		add_int(head, i);
		head += ',';
		add_str(head, player->get_name());
		head += ',';
		add_int(head, player->get_player_color1());
		head += ',';
		add_int(head, player->get_player_color2());
		head += ',';
		add_int(head, player->get_ai_id());
		head += ',';
		add_int(head, player->is_locked());
		rows.add(key_of(i), head, "", "");
	}
}

void info_depots(rows_t &rows, id_filter_t &ids)
{
	std::string head;
	for(  depot_t *depot : depot_t::get_depot_list()  ) {
		const koord3d pos = depot->get_pos();
		if(  !ids.wants(pos.x, pos.y, pos.z, 3)  ) {
			continue;
		}
		// [x, y, z, owner, waytype, name, hash]
		head = "[";
		head += key_of(pos);
		head += ',';
		add_int(head, player_nr_of(depot->get_owner()));
		head += ',';
		add_int(head, depot->get_waytype());
		head += ',';
		add_str(head, depot->get_name());
		rows.add(key_of(pos), head, "", "");
	}
}

int kind_index(const std::string &kind)
{
	for(  int i = 0;  i < (int)(sizeof(KINDS) / sizeof(KINDS[0]));  i++  ) {
		if(  kind == KINDS[i]  ) {
			return i;
		}
	}
	return -1;
}

void add_named(std::string &out, const char *name, sint64 value)
{
	if(  out.size() > 1  ) {
		out += ',';
	}
	add_str(out, name);
	out += ':';
	add_int(out, value);
}

/// the settings that the checks of the game's tools depend on, named as in settings_t (the savegame)
std::string info_settings(karte_t *welt)
{
	const settings_t &s = welt->get_settings();
	std::string obj = "{";
	add_named(obj, "use_timeline", welt->use_timeline());
	add_named(obj, "timeline_year_month", welt->get_timeline_year_month());   // 0 without timeline: everything is available
	add_named(obj, "current_month", welt->get_current_month());
	add_named(obj, "starting_year", s.get_starting_year());
	add_named(obj, "bits_per_month", s.get_bits_per_month());
	add_named(obj, "allow_buying_obsolete_vehicles", s.get_allow_buying_obsolete_vehicles());
	add_named(obj, "allow_overloading", s.is_allow_overloading());
	add_named(obj, "spacing_shift_divisor", s.get_spacing_shift_divisor());
	add_named(obj, "max_rail_convoi_length", s.get_max_rail_convoi_length());
	add_named(obj, "max_road_convoi_length", s.get_max_road_convoi_length());
	add_named(obj, "max_ship_convoi_length", s.get_max_ship_convoi_length());
	add_named(obj, "max_air_convoi_length", s.get_max_air_convoi_length());
	add_named(obj, "drive_on_left", s.is_drive_left());
	obj += '}';
	return obj;
}

std::string kinds_error(const char *extra)
{
	std::string msg = "kind must be one of halts, convoys, lines, factories, cities, players, depots";
	msg += extra;
	return error_json(msg.c_str());
}

} // namespace


std::string live_state::get_info(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	const std::string kind = mcp_server_t::json_get_string(args_json, "kind");
	if(  kind == "settings"  ) {
		std::string out = "{";
		live_state::add_head(out, welt);
		out += ",\"kind\":\"settings\",\"settings\":";
		out += info_settings(welt);
		out += '}';
		return out;
	}
	const int k = kind_index(kind);
	if(  k < 0  ) {
		return kinds_error(", settings");
	}
	const std::string level_name = mcp_server_t::json_get_string(args_json, "level");
	info_level_t level = LEVEL_BASIC;
	if(  level_name == "full"  ) {
		level = LEVEL_FULL;
	}
	else if(  level_name == "area"  ) {
		if(  kind != "halts"  ) {
			return error_json("level area is for halts only");
		}
		level = LEVEL_AREA;
	}
	else if(  !level_name.empty()  &&  level_name != "basic"  &&  level_name != "null"  ) {
		return error_json("level must be basic, full or area");
	}
	const bool hash_only = get_bool_arg(args_json, "hash_only");
	id_filter_t ids(mcp_server_t::json_get_raw(args_json, "ids"));

	rows_t rows(hash_only);
	bool need_vnames = false;
	switch(  k  ) {
		case 0: info_halts(rows, ids, welt, level); break;
		case 1: info_convoys(rows, ids, welt, level); need_vnames = !hash_only; break;
		case 2: info_lines(rows, ids, welt, level); break;
		case 3: info_factories(rows, ids, welt); break;
		case 4: info_cities(rows, ids, welt); break;
		case 5: info_players(rows, ids, welt); break;
		case 6: info_depots(rows, ids); break;
	}

	std::string out = "{";
	live_state::add_head(out, welt);
	out += ",\"kind\":";
	add_str(out, kind.c_str());
	out += ",\"level\":";
	add_str(out, level == LEVEL_FULL ? "full" : level == LEVEL_AREA ? "area" : "basic");
	out += ",\"hash_only\":";
	out += hash_only ? "true" : "false";
	out += ",\"rows\":[";
	out += rows.out;
	out += "],\"gone\":";
	out += ids.gone();
	if(  need_vnames  ) {
		out += ',';
		add_vehicle_names(out);
	}
	out += '}';
	return out;
}


// ---------------------------------------------------------------------------
// get_history
// ---------------------------------------------------------------------------

namespace {

/// [[f(0, 0), f(0, 1), ...], [f(1, 0), ...], ...]: n1 arrays of n2 values
void add_table(std::string &out, int n1, int n2, const std::function<sint64(int, int)> &f)
{
	out += '[';
	for(  int i = 0;  i < n1;  i++  ) {
		if(  i > 0  ) {
			out += ',';
		}
		out += '[';
		for(  int j = 0;  j < n2;  j++  ) {
			if(  j > 0  ) {
				out += ',';
			}
			add_int(out, f(i, j));
		}
		out += ']';
	}
	out += ']';
}

} // namespace


std::string live_state::get_history(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return error_json("world not ready");
	}
	const std::string kind = mcp_server_t::json_get_string(args_json, "kind");
	id_filter_t ids(mcp_server_t::json_get_raw(args_json, "ids"));

	std::string rows;
	const auto next_row = [&]() {
		if(  !rows.empty()  ) {
			rows += ',';
		}
	};

	if(  kind == "halts"  ) {
		// [id, [cost][month]] (month 0 = this month)
		for(  const halthandle_t &halt : haltestelle_t::get_alle_haltestellen()  ) {
			if(  !halt.is_bound()  ||  !ids.wants(halt.get_id())  ) {
				continue;
			}
			next_row();
			rows += '[';
			add_int(rows, halt.get_id());
			rows += ',';
			add_table(rows, MAX_HALT_COST, MAX_MONTHS, [&](int cost, int month) { return halt->get_finance_history(month, cost); });
			rows += ']';
		}
	}
	else if(  kind == "convoys"  ) {
		// [id, [cost][month], total distance]
		for(  const convoihandle_t &cnv : welt->convoys()  ) {
			if(  !cnv.is_bound()  ||  !ids.wants(cnv.get_id())  ) {
				continue;
			}
			next_row();
			rows += '[';
			add_int(rows, cnv.get_id());
			rows += ',';
			add_table(rows, convoi_t::MAX_CONVOI_COST, MAX_MONTHS, [&](int cost, int month) { return cnv->get_finance_history(month, cost); });
			rows += ',';
			add_int(rows, cnv->get_total_distance_traveled());
			rows += ']';
		}
	}
	else if(  kind == "lines"  ) {
		// [id, [cost][month]]
		vector_tpl<linehandle_t> lines;
		for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
			player_t *player = welt->get_player(i);
			if(  !player  ) {
				continue;
			}
			lines.clear();
			player->simlinemgmt.get_lines(simline_t::line, &lines);
			for(  const linehandle_t &line : lines  ) {
				if(  !ids.wants(line.get_id())  ) {
					continue;
				}
				next_row();
				rows += '[';
				add_int(rows, line.get_id());
				rows += ',';
				add_table(rows, MAX_LINE_COST, MAX_MONTHS, [&](int cost, int month) { return line->get_finance_history(month, cost); });
				rows += ']';
			}
		}
	}
	else if(  kind == "cities"  ) {
		// [x, y, [year][type], [month][type]]
		for(  stadt_t *city : welt->get_cities()  ) {
			const koord pos = city->get_pos();
			if(  !ids.wants(pos.x, pos.y, 0, 2)  ) {
				continue;
			}
			next_row();
			rows += '[';
			rows += key_of(koord3d(pos, 0), false);
			rows += ',';
			add_table(rows, MAX_CITY_HISTORY_YEARS, MAX_CITY_HISTORY, [&](int year, int type) { return city->get_finance_history_year(year, type); });
			rows += ',';
			add_table(rows, MAX_CITY_HISTORY_MONTHS, MAX_CITY_HISTORY, [&](int month, int type) { return city->get_finance_history_month(month, type); });
			rows += ']';
		}
	}
	else if(  kind == "factories"  ) {
		// [x, y, z, [type][month], [[type][month] of each input], [[type][month] of each output]]
		for(  fabrik_t *fab : welt->get_fab_list()  ) {
			const koord3d pos = fab->get_pos();
			if(  !ids.wants(pos.x, pos.y, pos.z, 3)  ) {
				continue;
			}
			next_row();
			rows += '[';
			rows += key_of(pos);
			rows += ',';
			add_table(rows, MAX_FAB_STAT, MAX_MONTH, [&](int type, int month) { return fab->get_stat(month, type); });
			for(  int io = 0;  io < 2;  io++  ) {
				const array_tpl<ware_production_t> &list = io == 0 ? fab->get_input() : fab->get_output();
				rows += ",[";
				for(  uint32 i = 0;  i < list.get_count();  i++  ) {
					if(  i > 0  ) {
						rows += ',';
					}
					add_table(rows, MAX_FAB_GOODS_STAT, MAX_MONTH, [&](int type, int month) { return list[i].get_stat(month, type); });
				}
				rows += ']';
			}
			rows += ']';
		}
	}
	else if(  kind == "players"  ) {
		// [nr, [year][company type], [month][company type], [transport type][year][vehicle type], [transport type][month][vehicle type]]
		for(  uint8 nr = 0;  nr < MAX_PLAYER_COUNT;  nr++  ) {
			const player_t *player = welt->get_player(nr);
			if(  !player  ||  !ids.wants(nr)  ) {
				continue;
			}
			const finance_t *finance = player->get_finance();
			next_row();
			rows += '[';
			add_int(rows, nr);
			rows += ',';
			add_table(rows, MAX_PLAYER_HISTORY_YEARS, ATC_MAX, [&](int year, int type) { return finance->get_history_com_year(year, type); });
			rows += ',';
			add_table(rows, MAX_PLAYER_HISTORY_MONTHS, ATC_MAX, [&](int month, int type) { return finance->get_history_com_month(month, type); });
			for(  int period = 0;  period < 2;  period++  ) {
				rows += ",[";
				for(  int tt = 0;  tt < TT_MAX;  tt++  ) {
					if(  tt > 0  ) {
						rows += ',';
					}
					if(  period == 0  ) {
						add_table(rows, MAX_PLAYER_HISTORY_YEARS, ATV_MAX, [&](int year, int type) { return finance->get_history_veh_year((transport_type)tt, year, type); });
					}
					else {
						add_table(rows, MAX_PLAYER_HISTORY_MONTHS, ATV_MAX, [&](int month, int type) { return finance->get_history_veh_month((transport_type)tt, month, type); });
					}
				}
				rows += ']';
			}
			rows += ']';
		}
	}
	else if(  kind == "world"  ) {
		// one row: [[year][type], [month][type], [decade][type]]
		rows += '[';
		add_table(rows, MAX_WORLD_HISTORY_YEARS, karte_t::MAX_WORLD_COST, [&](int year, int type) { return welt->get_finance_history_year(year, type); });
		rows += ',';
		add_table(rows, MAX_WORLD_HISTORY_MONTHS, karte_t::MAX_WORLD_COST, [&](int month, int type) { return welt->get_finance_history_month(month, type); });
		rows += ',';
		add_table(rows, MAX_WORLD_HISTORY_DECADES, karte_t::MAX_WORLD_COST, [&](int decade, int type) { return welt->get_finance_history_decade(decade, type); });
		rows += ']';
	}
	else {
		return kinds_error(", world");
	}

	std::string out = "{";
	live_state::add_head(out, welt);
	out += ",\"kind\":";
	add_str(out, kind.c_str());
	out += ",\"month\":";
	add_int(out, welt->get_current_month());
	out += ",\"rows\":[";
	out += rows;
	out += "],\"gone\":";
	out += kind == "world" ? "[]" : ids.gone();
	out += '}';
	return out;
}
