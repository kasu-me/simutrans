/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * The world sections of get_live_state: halts, factories, cities, players,
 * depots, lines and convoy schedules. Each is sent on request only; with
 * `since`, only what changed since the caller's previous answer
 * (see live_util::entity_changes_t).
 */

#include "live_util.h"
#include "mcp_server.h"

#include <map>

#include "../simworld.h"
#include "../simconvoi.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simfab.h"
#include "../simcity.h"
#include "../simdepot.h"
#include "../simline.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../player/simplay.h"
#include "../dataobj/schedule.h"
#include "../display/simgraph.h"
#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/factory_desc.h"
#include "../descriptor/vehicle_desc.h"

using namespace live_util;


namespace {

uint64 pos_key(koord3d k)
{
	return ((uint64)(uint16)k.x << 32) | ((uint64)(uint16)k.y << 16) | (uint8)k.z;
}

void add_pos_key(std::string &out, uint64 key)
{
	out += '[';
	add_int(out, (sint16)(uint16)(key >> 32));
	out += ',';
	add_int(out, (sint16)(uint16)(key >> 16));
	out += ',';
	add_int(out, (sint8)(uint8)key);
	out += ']';
}

void add_id_key(std::string &out, uint64 key)
{
	add_int(out, (sint64)key);
}

/**
 * Writes "kind":[records],"kind_gone":[keys],"kind_full":bool.
 * @param each called with a function to hand every (key, record) to
 */
template<class EACH, class KEY>
void add_section(std::string &out, const char *kind, bool has_since, uint32 since, EACH each, KEY add_key)
{
	entity_changes_t &changes = changes_of(kind);
	changes.begin(has_since, since);
	out += ",\"";
	out += kind;
	out += "\":[";
	bool first = true;
	each( [&](uint64 key, const std::string &record) {
		if(  changes.update(key, record)  ) {
			if(  !first  ) {
				out += ',';
			}
			first = false;
			out += record;
		}
	} );
	out += "],\"";
	out += kind;
	out += "_gone\":[";
	first = true;
	for(  uint64 key : changes.end()  ) {
		if(  !first  ) {
			out += ',';
		}
		first = false;
		add_key(out, key);
	}
	out += "],\"";
	out += kind;
	out += "_full\":";
	out += changes.is_full() ? "true" : "false";
}

/// the status colour of a halt (haltestelle_t::recalc_status): 0 green, 1 yellow, 2 orange, 3 red
int halt_status(const haltestelle_t *halt)
{
	const PIXVAL c = halt->get_status_farbe();
	if(  c == color_idx_to_rgb(COL_RED)  ) {
		return 3;
	}
	if(  c == color_idx_to_rgb(COL_ORANGE)  ) {
		return 2;
	}
	if(  c == color_idx_to_rgb(COL_YELLOW)  ) {
		return 1;
	}
	return 0;
}

template<class T> uint32 hash_value(uint32 h, T v)
{
	return fnv1a((const char *)&v, sizeof(v), h);
}

/// hash of what schedule_t::sprintf_schedule writes, without the current stop (which changes at every stop)
uint32 schedule_hash(const schedule_t *schedule)
{
	if(  !schedule  ) {
		return 0;
	}
	uint32 h = 2166136261u;
	h = hash_value(h, schedule->get_flags());
	h = hash_value(h, schedule->get_max_speed());
	h = hash_value(h, schedule->get_departure_slot_group_id().get_id());
	h = hash_value(h, schedule->get_additional_base_waiting_time());
	h = hash_value(h, (int)schedule->get_type());
	h = hash_value(h, schedule->get_next_line().get_id());
	for(  const schedule_entry_t &e : schedule->get_entries()  ) {
		h = hash_value(h, e.pos.x);
		h = hash_value(h, e.pos.y);
		h = hash_value(h, e.pos.z);
		h = hash_value(h, e.minimum_loading);
		h = hash_value(h, e.waiting_time_shift);
		h = hash_value(h, e.get_stop_flags());
		h = hash_value(h, e.max_speed_kmh_of_convoi);
		h = hash_value(h, e.spacing);
		h = hash_value(h, e.spacing_shift);
		h = hash_value(h, e.delay_tolerance);
		h = hash_value(h, e.length_coupling_done);
		h = hash_value(h, e.maximum_loading);
		h = hash_value(h, e.balance_speed_kmh_of_convoi);
	}
	return h;
}

int player_nr_of(const player_t *player)
{
	return player ? player->get_player_nr() : -1;
}

} // namespace


void live_util::add_world_sections(std::string &out, karte_t *welt, const std::string &args_json, bool has_since, uint32 since, bool &need_vnames)
{
	const bool halts = get_bool_arg(args_json, "halts");
	const bool factories = get_bool_arg(args_json, "factories");

	if(  halts  ) {
		// [id, owner, waiting total, [[via halt id, goods index, amount], ...], status]
		add_section(out, "halts", has_since, since, [&](auto emit) {
			std::map<std::pair<uint32, uint8>, uint32> via;
			std::string rec;
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
				rec = "[";
				add_int(rec, halt.get_id());
				rec += ',';
				add_int(rec, player_nr_of(halt->get_owner()));
				rec += ',';
				add_int(rec, total);
				rec += ",[";
				bool first = true;
				for(  const auto &v : via  ) {
					if(  !first  ) {
						rec += ',';
					}
					first = false;
					rec += '[';
					add_int(rec, v.first.first);
					rec += ',';
					add_int(rec, v.first.second);
					rec += ',';
					add_int(rec, v.second);
					rec += ']';
				}
				rec += "],";
				add_int(rec, halt_status(halt.get_rep()));
				rec += ']';
				emit(halt.get_id(), rec);
			}
		}, add_id_key);
	}

	if(  halts  ||  factories  ) {
		out += ",\"goods\":[";
		for(  uint16 i = 0;  i < goods_manager_t::get_count();  i++  ) {
			if(  i > 0  ) {
				out += ',';
			}
			add_str(out, goods_manager_t::get_info(i)->get_name());
		}
		out += ']';
	}

	if(  factories  ) {
		// [x, y, z, name, owner, status, production, production factor, [[goods index, stock, max, in transit], ...inputs], [[goods index, stock, max], ...outputs]]
		add_section(out, "factories", has_since, since, [&](auto emit) {
			std::string rec;
			for(  fabrik_t *fab : welt->get_fab_list()  ) {
				const koord3d pos = fab->get_pos();
				rec = "[";
				add_int(rec, pos.x);
				rec += ',';
				add_int(rec, pos.y);
				rec += ',';
				add_int(rec, pos.z);
				rec += ',';
				add_str(rec, fab->get_desc()->get_name());
				rec += ',';
				add_int(rec, player_nr_of(fab->get_owner()));
				rec += ',';
				add_int(rec, fab->get_status());
				rec += ',';
				add_int(rec, fab->get_current_production());
				rec += ',';
				add_int(rec, fab->get_prodfactor());
				rec += ",[";
				bool first = true;
				for(  const ware_production_t &w : fab->get_input()  ) {
					if(  !first  ) {
						rec += ',';
					}
					first = false;
					rec += '[';
					add_int(rec, w.get_typ()->get_index());
					rec += ',';
					add_int(rec, w.menge >> fabrik_t::precision_bits);
					rec += ',';
					add_int(rec, w.max >> fabrik_t::precision_bits);
					rec += ',';
					add_int(rec, w.get_in_transit());
					rec += ']';
				}
				rec += "],[";
				first = true;
				for(  const ware_production_t &w : fab->get_output()  ) {
					if(  !first  ) {
						rec += ',';
					}
					first = false;
					rec += '[';
					add_int(rec, w.get_typ()->get_index());
					rec += ',';
					add_int(rec, w.menge >> fabrik_t::precision_bits);
					rec += ',';
					add_int(rec, w.max >> fabrik_t::precision_bits);
					rec += ']';
				}
				rec += "]]";
				emit(pos_key(pos), rec);
			}
		}, add_pos_key);
	}

	if(  get_bool_arg(args_json, "cities")  ) {
		// [x, y, name, population] (x, y: the town hall)
		add_section(out, "cities", has_since, since, [&](auto emit) {
			std::string rec;
			for(  stadt_t *city : welt->get_cities()  ) {
				const koord pos = city->get_pos();
				rec = "[";
				add_int(rec, pos.x);
				rec += ',';
				add_int(rec, pos.y);
				rec += ',';
				add_str(rec, city->get_name());
				rec += ',';
				add_int(rec, city->get_einwohner());
				rec += ']';
				emit(pos_key(koord3d(pos, 0)), rec);
			}
		}, add_pos_key);
	}

	if(  get_bool_arg(args_json, "players")  ) {
		// [nr, name, cash (credits), colour 1, colour 2 (palette indices), type (ai id, 0 human), locked]
		add_section(out, "players", has_since, since, [&](auto emit) {
			std::string rec;
			for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
				const player_t *player = welt->get_player(i);
				if(  !player  ) {
					continue;
				}
				rec = "[";
				add_int(rec, i);
				rec += ',';
				add_str(rec, player->get_name());
				rec += ',';
				add_double(rec, player->get_account_balance_as_double(), 2);
				rec += ',';
				add_int(rec, player->get_player_color1());
				rec += ',';
				add_int(rec, player->get_player_color2());
				rec += ',';
				add_int(rec, player->get_ai_id());
				rec += ',';
				add_int(rec, player->is_locked());
				rec += ']';
				emit(i, rec);
			}
		}, add_id_key);
	}

	if(  get_bool_arg(args_json, "depots")  ) {
		// [x, y, z, owner, waytype, [convoy ids], [vehicle type (vnames index), ...]] (the vehicles that are not in a convoy)
		need_vnames = true;
		add_section(out, "depots", has_since, since, [&](auto emit) {
			std::string rec;
			for(  depot_t *depot : depot_t::get_depot_list()  ) {
				const koord3d pos = depot->get_pos();
				rec = "[";
				add_int(rec, pos.x);
				rec += ',';
				add_int(rec, pos.y);
				rec += ',';
				add_int(rec, pos.z);
				rec += ',';
				add_int(rec, player_nr_of(depot->get_owner()));
				rec += ',';
				add_int(rec, depot->get_waytype());
				rec += ",[";
				bool first = true;
				for(  const convoihandle_t &cnv : depot->get_convoy_list()  ) {
					if(  !first  ) {
						rec += ',';
					}
					first = false;
					add_int(rec, cnv.get_id());
				}
				rec += "],[";
				first = true;
				for(  const vehicle_t *v : depot->get_vehicle_list()  ) {
					if(  !first  ) {
						rec += ',';
					}
					first = false;
					add_int(rec, vehicle_name_index(v->get_desc()));
				}
				rec += "]]";
				emit(pos_key(pos), rec);
			}
		}, add_pos_key);
	}

	if(  get_bool_arg(args_json, "lines")  ) {
		// [id, owner, name, line type, [convoy ids], schedule hash]
		add_section(out, "lines", has_since, since, [&](auto emit) {
			std::string rec;
			vector_tpl<linehandle_t> lines;
			for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
				player_t *player = welt->get_player(i);
				if(  !player  ) {
					continue;
				}
				lines.clear();
				player->simlinemgmt.get_lines(simline_t::line, &lines);
				for(  const linehandle_t &line : lines  ) {
					rec = "[";
					add_int(rec, line.get_id());
					rec += ',';
					add_int(rec, i);
					rec += ',';
					add_str(rec, line->get_name());
					rec += ',';
					add_int(rec, line->get_linetype());
					rec += ",[";
					bool first = true;
					for(  const convoihandle_t &cnv : line->get_convoys()  ) {
						if(  !first  ) {
							rec += ',';
						}
						first = false;
						add_int(rec, cnv.get_id());
					}
					rec += "],";
					add_int(rec, schedule_hash(line->get_schedule()));
					rec += ']';
					emit(line.get_id(), rec);
				}
			}
		}, add_id_key);

		// [convoy id, line id (-1 none), schedule hash]: a convoy appears when its schedule or line changed
		add_section(out, "schedules", has_since, since, [&](auto emit) {
			std::string rec;
			for(  const convoihandle_t &cnv : welt->convoys()  ) {
				if(  !cnv.is_bound()  ) {
					continue;
				}
				rec = "[";
				add_int(rec, cnv.get_id());
				rec += ',';
				add_int(rec, cnv->get_line().is_bound() ? (sint64)cnv->get_line().get_id() : -1);
				rec += ',';
				add_int(rec, schedule_hash(cnv->get_schedule()));
				rec += ']';
				emit(cnv.get_id(), rec);
			}
		}, add_id_key);
	}
}
