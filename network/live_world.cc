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

#include <algorithm>
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
void add_section(std::string &out, const char *kind, bool has_since, uint32 since, EACH each, KEY add_key, const char *cache_key = NULL)
{
	// records of another shape are compared separately
	entity_changes_t &changes = changes_of(cache_key ? cache_key : kind);
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

/// at most this many final destinations per via halt and goods (halts_dest)
const size_t MAX_DESTINATIONS = 16;

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
		// [id, owner, waiting total, [[via halt id, goods index, amount (, [[destination halt id, amount], ...])], ...], status]
		const bool dest = get_bool_arg(args_json, "halts_dest");
		add_section(out, "halts", has_since, since, [&](auto emit) {
			std::map<std::pair<uint32, uint8>, uint32> via;
			std::map<std::pair<uint32, uint8>, std::map<uint32, uint32>> dests;
			std::vector<std::pair<uint32, uint32>> sorted;
			std::string rec;
			for(  const halthandle_t &halt : haltestelle_t::get_alle_haltestellen()  ) {
				if(  !halt.is_bound()  ) {
					continue;
				}
				via.clear();
				dests.clear();
				uint32 total = 0;
				halt->for_each_waiting_ware( [&](const ware_t &w) {
					const std::pair<uint32, uint8> key(w.get_zwischenziel().get_id(), w.get_index());
					via[key] += w.menge;
					if(  dest  ) {
						dests[key][w.get_ziel().get_id()] += w.menge;
					}
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
					if(  dest  ) {
						// the final destinations, the largest amounts first
						sorted.assign(dests[v.first].begin(), dests[v.first].end());
						std::sort(sorted.begin(), sorted.end(), [](const std::pair<uint32, uint32> &a, const std::pair<uint32, uint32> &b) {
							return a.second != b.second ? a.second > b.second : a.first < b.first;
						} );
						rec += ",[";
						for(  size_t i = 0;  i < sorted.size()  &&  i < MAX_DESTINATIONS;  i++  ) {
							if(  i > 0  ) {
								rec += ',';
							}
							rec += '[';
							add_int(rec, sorted[i].first);
							rec += ',';
							add_int(rec, sorted[i].second);
							rec += ']';
						}
						rec += ']';
					}
					rec += ']';
				}
				rec += "],";
				add_int(rec, halt_status(halt.get_rep()));
				rec += ']';
				emit(halt.get_id(), rec);
			}
		}, add_id_key, dest ? "halts+dest" : NULL);
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
		// [x, y, z, name, owner, status, production, production factor, [[goods index, stock, max, in transit], ...inputs], [[goods index, stock, max], ...outputs], rotation]
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
				rec += "],";
				add_int(rec, fab->get_rotate());
				rec += ']';
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
		// [id, owner, name, line type, [convoy ids], schedule hash, colour (palette index)]
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
					rec += ',';
					add_int(rec, line->get_colour());
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
