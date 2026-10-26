/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_line.cc exports line related functions. */

#include "get_next.h"

#include "api_obj_desc_base.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../simworld.h"
#include "../../player/simplay.h"

// for manipulation of lines
#include "../../simmenu.h"
#include "../../dataobj/schedule.h"
#include "../../network/live_util.h"

// template<> schedule_t* script_api::param<schedule_t*>::get(HSQUIRRELVM, SQInteger);

using namespace script_api;

vector_tpl<sint64> const& get_line_stat(simline_t *line, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (line  &&  0<=INDEX  &&  INDEX<MAX_LINE_COST) {
		for(uint16 i = 0; i < MAX_MONTHS; i++) {
			v.append( line->get_stat_converted(i, INDEX) );
		}
	}
	return v;
}


waytype_t line_way_type(simline_t *line)
{
	if (line) {
		switch (line->get_linetype()) {
			case simline_t::truckline: return road_wt;
			case simline_t::trainline: return track_wt;
			case simline_t::shipline: return water_wt;
			case simline_t::monorailline: return monorail_wt;
			case simline_t::maglevline: return maglev_wt;
			case simline_t::narrowgaugeline: return narrowgauge_wt;
			case simline_t::airline: return air_wt;
			case simline_t::tramline: return tram_wt;
			default: ;
		}
	}
	return invalid_wt;
}

// implementation in api_schedule.cc
const char *script_schedule_rejection(const schedule_t *sched);

/// why change_schedule would fail, NULL if it would not
static const char *line_schedule_error(simline_t* line, player_t *player, schedule_t *sched)
{
	if (sched == NULL) {
		return "invalid: Invalid schedule provided";
	}
	if (player  &&  !player_t::check_owner(line->get_owner(), player)) {
		return "permission: the object belongs to another player";
	}
	// all or nothing: the entries that are no stops would be left out
	if (const char *err = script_schedule_rejection(sched)) {
		return err;
	}
	// the tool would ignore it silently (schedule_t::sscanf_schedule)
	if (line->get_schedule()  &&  line->get_schedule()->get_type() != sched->get_type()) {
		return "invalid: the schedule is for another waytype";
	}
	return NULL;
}

const char *line_validate_schedule(simline_t* line, player_t *player, schedule_t *sched)
{
	return line_schedule_error(line, player, sched);
}

call_tool_init line_change_schedule(simline_t* line, player_t *player, schedule_t *sched)
{
	if (const char *err = line_schedule_error(line, NULL, sched)) {
		return err;
	}
	// build param string (see line_management_gui_t::infowin_event)
	cbuffer_t buf;
	buf.printf( "g,%i,", line->get_handle().get_id() );
	sched->sprintf_schedule( buf );

	call_tool_init call(TOOL_CHANGE_LINE | SIMPLE_TOOL, buf, 0, player);
	call.require_owner(line->get_owner());
	return call;
}

call_tool_init line_delete(simline_t* line, player_t *player)
{
	if (line->count_convoys() == 0) {
		cbuffer_t buf;
		buf.printf( "d,%i", line->get_handle().get_id() );

		call_tool_init call(TOOL_CHANGE_LINE | SIMPLE_TOOL, buf, 0, player);
		call.require_owner(line->get_owner());
		return call;
	}
	return "invalid: Cannot delete lines with associated convoys";
}

SQInteger line_export_convoy_list(HSQUIRRELVM vm)
{
	linehandle_t line = param<linehandle_t>::get(vm, 1);
	if (line.is_bound()) {
		push_instance(vm, "convoy_list_x");
		set_slot(vm, "line_id", line.get_id());
		return 1;
	}
	sq_raise_error(vm, "Invalid line handle provided");
	return SQ_ERROR;
}

uint32 line_get_schedule_hash(simline_t *line)
{
	return live_util::schedule_hash(line->get_schedule());
}

call_tool_init line_set_name(simline_t* line, const char* name)
{
	if (name == NULL) {
		return "invalid: no name";
	}
	call_tool_init call = command_rename(line->get_owner(), 'l', line->get_handle().get_id(), name);
	call.require_owner(line->get_owner());
	return call;
}

vector_tpl<linehandle_t> const* generic_get_line_list(HSQUIRRELVM vm, SQInteger index)
{
	uint32 id;
	if (SQ_SUCCEEDED(get_slot(vm, "halt_id", id, index))) {
		halthandle_t halt;
		halt.set_id(id);
		if (halt.is_bound()) {
			return &halt->registered_lines;
		}
	}
	if (SQ_SUCCEEDED(get_slot(vm, "player_id", id, index))  &&  id < PLAYER_UNOWNED) {
		if (player_t *player = welt->get_player(id)) {
			return &player->simlinemgmt.get_line_list();
		}
	}
	sq_raise_error(vm, "Invalid line list.");
	return NULL;
}

SQInteger generic_get_next_line(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	return list ? generic_get_next(vm, list->get_count()) : SQ_ERROR;
}

SQInteger generic_get_line_by_index(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	sint32 index = param<sint32>::get(vm, 2);
	linehandle_t line;
	if (list) {
		line = (0<=index  &&  (uint32)index < list->get_count()) ?  (*list)[index] : linehandle_t();
		return push_instance(vm, "line_x",  line.is_bound() ? line.get_id() :  0);
	}
	return SQ_ERROR;
}

SQInteger generic_get_line_count(HSQUIRRELVM vm)
{
	vector_tpl<linehandle_t> const* list = generic_get_line_list(vm, 1);
	return param<uint32>::push(vm, list ? list->get_count() : 0);
}

void export_line(HSQUIRRELVM vm)
{
	/**
	 * Implements iterator to iterate through lists of lines.
	 *
	 * Usage:
	 * @code
	 * local list = player_x(0).get_line_list()
	 * foreach(line in list) {
	 *     ... // line is an instance of the line_x class
	 * }
	 * @endcode
	 *
	 * @see player_x::get_line_list, halt_x::get_line_list
	 */
	begin_class(vm, "line_list_x", 0);
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 */
	register_function(vm, generic_get_next_line,     "_nexti",  2, "x o|i");
	/**
	 * Meta-method to be used in foreach loops. Do not call them directly.
	 * @typemask line_x()
	 */
	register_function(vm, generic_get_line_by_index, "_get",    2, "xi");
	/**
	 * Returns number of lines in the list.
	 * @typemask integer()
	 */
	register_function(vm, generic_get_line_count, "get_count",  1, "x");
	end_class(vm);

	/**
	 * Class to access lines.
	 */
	begin_class(vm, "line_x", "extend_get,ingame_object");

	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<simline_t*>(vm); //register_function("is_valid")
	/**
	 * Line name.
	 * @returns name
	 */
	register_method(vm, &simline_t::get_name, "get_name");
	/**
	 * Sets line name.
	 * @ingroup rename_func
	 * @typemask void(string)
	 */
	register_method(vm, &line_set_name, "set_name", true);
	/**
	 * Line owner.
	 * @returns owner
	 */
	register_method(vm, &simline_t::get_owner, "get_owner");
	/**
	 * Schedule of this line.
	 */
	register_method(vm, &simline_t::get_schedule, "get_schedule");
	/**
	 * Hash of the schedule without its current stop, see convoy_x::get_schedule_hash.
	 * @returns hash (FNV-1a, 32 bit)
	 */
	register_method(vm, &line_get_schedule_hash, "get_schedule_hash", true);
	/**
	 * Returns array of goods categories that can be transported by this convoy.
	 * @returns array
	 */
	register_method(vm, &simline_t::get_goods_catg_index, "get_goods_catg_index");
	/**
	 * Get monthly statistics of capacity.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_capacity",          freevariable<sint32>(LINE_CAPACITY), true);
	/**
	 * Get monthly statistics of number of transported goods.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_transported_goods", freevariable<sint32>(LINE_TRANSPORTED_GOODS), true );
	/**
	 * Get monthly statistics of number of convoys in this line.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_convoy_count",      freevariable<sint32>(LINE_CONVOIS), true );
	/**
	 * Get monthly statistics of revenue.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_revenue",           freevariable<sint32>(LINE_REVENUE), true );
	/**
	 * Get monthly statistics of running costs.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_cost",              freevariable<sint32>(LINE_OPERATIONS), true );
	/**
	 * Get monthly statistics of profit.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_profit",            freevariable<sint32>(LINE_PROFIT), true );
	/**
	 * Get monthly statistics of traveled distance.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_traveled_distance", freevariable<sint32>(LINE_DISTANCE), true );
	/**
	 * Get monthly statistics of income/loss due to way tolls.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_line_stat, "get_way_tolls",         freevariable<sint32>(LINE_WAYTOLL), true );
	/**
	 * Exports list of convoys belonging to this line.
	 * @typemask convoy_list_x()
	 */
	register_function(vm, &line_export_convoy_list, "get_convoy_list", 1, param<linehandle_t>::typemask());

	/**
	 * @return waytype of the line
	 */
	register_method(vm, &line_way_type, "get_waytype", true);

	/**
	 * Change schedule of line.
	 * Entries that cannot be stops of this line fail the whole call (nothing is changed).
	 * The recorded journey, waiting and stopping times are kept for the entries whose
	 * position and previous position did not change (as in the schedule window).
	 * @param pl player to pay for the change
	 * @param sched new schedule
	 * @returns as all game commands: true when executed at once, false when sent to the server
	 *          (network game); a failure raises an error (see validate_schedule)
	 * @ingroup game_cmd
	 */
	register_method(vm, line_change_schedule, "change_schedule", true);

	/**
	 * Check a schedule as change_schedule does, without changing anything.
	 * @param pl player who would change it
	 * @param sched the schedule
	 * @returns null if change_schedule would accept it, else the error it would raise
	 */
	register_method(vm, line_validate_schedule, "validate_schedule", true);

	/**
	 * Delete line
	 * @ingroup game_cmd
	 */
	register_method(vm, line_delete, "destroy", true);

	end_class(vm);
}
