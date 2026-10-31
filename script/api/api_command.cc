/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_command.cc exports the command_x class, which encodes the tools to manipulate a game */

#include "api_command.h"
#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "../api_param.h"
#include "../api_class.h"
#include "../api_function.h"
#include "../../simmenu.h"
#include "../../simtool.h"
#include "../../simworld.h"
#include "../../dataobj/environment.h"
#include "../../script/script.h"
#include "../../descriptor/bridge_desc.h"
#include "../../descriptor/tunnel_desc.h"
#include "../../descriptor/building_desc.h"
#include "../../descriptor/roadsign_desc.h"
#include "../../descriptor/way_obj_desc.h"
#include "../../network/live_state.h"
#include "../../simconvoi.h"
#include "../../simfab.h"
#include "../../simhalt.h"
#include "../../simline.h"
#include "../../boden/grund.h"
#include "../../obj/roadsign.h"
#include "../../obj/signal.h"
#include "../../player/finance.h"
#include "../../player/simplay.h"
#include "../../bauer/wegbauer.h"
#include "../../bauer/brueckenbauer.h"
#include "../../bauer/tunnelbauer.h"
#include "../../boden/wege/strasse.h"
#include "../../obj/wayobj.h"
#include "../../obj/leitung2.h"
#include "../../dataobj/route.h"
#include "../../dataobj/scenario.h"
#include "../../simintr.h"
#include "../../sys/simsys.h"

#include <memory> // auto_ptr

using namespace script_api;


/**
 * The simple tools that command_x accepts, run by exec(): the tools of the game's windows that
 * change the game, with the commands (letters of the default_param) exec() allows, NULL: the
 * default_param has none. The other simple tools (pause, zoom, display, quit, screenshot ...)
 * concern the screen or the progress of the game, and stay refused.
 */
static const struct {
	uint16 id;
	const char *commands;
} exec_tools[] = {
	{ TOOL_CHANGE_CONVOI        | SIMPLE_TOOL, "nsdyDYretcmbiukao" },
	{ TOOL_CHANGE_LINE          | SIMPLE_TOOL, "pmoOtuw" },
	{ TOOL_CHANGE_HALT          | SIMPLE_TOOL, "cpmwl" },
	{ TOOL_HALT_PERMISSION      | SIMPLE_TOOL, NULL },
	{ TOOL_CHANGE_FACTORY       | SIMPLE_TOOL, "ds" },
	{ TOOL_CHANGE_PLAYER        | SIMPLE_TOOL, "c$nd" },
	{ TOOL_RECOLOUR_TOOL        | SIMPLE_TOOL, "12" },
	{ TOOL_SENDING_MONEY        | SIMPLE_TOOL, NULL },
	{ TOOL_MERGE_PLAYER         | SIMPLE_TOOL, NULL },
	{ TOOL_UNDO                 | SIMPLE_TOOL, NULL },
	{ TOOL_CHANGE_TRAFFIC_LIGHT | SIMPLE_TOOL, NULL },
	// the last eight have functions of their own (set_guide_signal ...)
	{ TOOL_CHANGE_ROADSIGN      | SIMPLE_TOOL, "rcgDnewsoatdplm" }
};

/// the entry of exec_tools, NULL if this tool cannot be run by exec()
static const char *const *exec_tool_commands(uint16 id)
{
	for (size_t i = 0; i < lengthof(exec_tools); i++) {
		if (exec_tools[i].id == id) {
			return &exec_tools[i].commands;
		}
	}
	return NULL;
}


SQInteger command_constructor(HSQUIRRELVM vm)
{
	// create tool
	uint16 id = param<uint16>::get(vm, 2);

	if (id & GENERAL_TOOL) {
		// we do not want scripts to open dialogues or quitting the game etc

		if (id == (TOOL_EXEC_TWO_CLICK_SCRIPT | GENERAL_TOOL)) {
			// do not create & attach instance, will be done separately
			return 0;
		}
		else {
			if (tool_t *tool = create_tool(id)) {
				my_tool_t* mtool = new my_tool_t(tool);
				attach_instance(vm, 1, mtool);
				return 0;
			}
		}
	}
	else if (exec_tool_commands(id)) {
		// a tool of the game's windows, see exec
		if (tool_t *tool = create_tool(id)) {
			my_tool_t* mtool = new my_tool_t(tool);
			attach_instance(vm, 1, mtool);
			return 0;
		}
	}
	return sq_raise_error(vm, "Invalid tool called (%d / 0x%x)", id & 0xfff, id);
}


SQInteger param<call_tool_init>::push(HSQUIRRELVM vm, call_tool_init v)
{
	if (v.error) {
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occurred");
	}
	// create tool, if necessary, delete on exit
	std::unique_ptr<tool_t> our_tool;
	tool_t *tool = v.tool;
	if (tool == NULL) {
		our_tool.reset(v.create_tool());
		tool = our_tool.get();
	}
	if (tool == NULL) {
		return sq_raise_error(vm, "Called null tool");
	}
	// set correct flags
	tool->flags &= (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	tool->flags |= tool_t::WFL_SCRIPT;
	// get player parameter
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = v.player;
		// must be scenario - set flag
		tool->flags |= tool_t::WFL_NO_CHK;
	}
	// sanity check
	if (player == NULL) {
		return sq_raise_error(vm, "Called tool with player == null");
	}
	if (v.required_owner  &&  !player_t::check_owner(v.required_owner, player)) {
		return sq_raise_error(vm, "permission: the object belongs to another player");
	}
	if (v.paying_player  &&  !v.paying_player->can_afford(v.cost)  &&  script_vm_t::is_mcp(vm)) {
		return sq_raise_error(vm, "funds: Insufficient funds!");
	}
	// check if calling suspendable tools is blocked
	if (const char* blocker = env_t::networkmode ? sq_get_suspend_blocker(vm) : NULL) {
		return sq_raise_error(vm, "busy: Cannot call this tool from within `%s'.", blocker);
	}

	// register this tool call for callback with this id
	uint32 callback_id = suspended_scripts_t::get_unique_key(tool);
	tool->callback_id = callback_id;

	const char *init_error = NULL;
	// HACK call karte_t::set_tool
	{
		// a tool of an MCP script that runs at once is reported to it (in network games: nwc_tool_t::do_command)
		live_changes_t::tool_op_t op(player);
		welt->set_tool(tool, player);
		if (!env_t::networkmode  &&  script_vm_t::is_mcp(vm)) {
			init_error = tool->get_init_error();
			op.done(vm, tool->get_id(), tool->get_default_param(), init_error);
		}
	}
	// in networkmode, call is suspended

	if (env_t::networkmode) {
		// register for wakeup
		suspended_scripts_t::register_suspended_script(callback_id, vm);
		// suspend vm for now, after wakeup it returns to the script
		return sq_suspendvm(vm);
	}
	else if (script_vm_t::is_mcp(vm)) {
		// null on success, as in network games (nwc_tool_t::do_command)
		if (init_error) {
			return param<const char*>::push(vm, script_vm_t::with_error_word(vm, init_error));
		}
		return 0;
	}
	else {
		bool res = true;
		return param<bool>::push(vm, res);
	}
}


call_tool_init script_api::command_rename(player_t *owner, char what, koord3d pos, const char* name)
{
	// build param string (see tool_rename_t::init)
	cbuffer_t buf;
	buf.printf( "%c%s,%s", what, pos.get_str(), name);

	// empty name? trigger error by setting id to zero
	uint16 tool_id = name ? TOOL_RENAME | SIMPLE_TOOL : 0;

	return call_tool_init(tool_id, (const char*)buf, 0, owner);
}

call_tool_init script_api::command_rename(player_t *owner, char what, uint32 id, const char* name)
{
	// build param string (see tool_rename_t::init)
	cbuffer_t buf;
	buf.printf( "%c%u,%s", what, id, name);

	// empty name? trigger error by setting id to zero
	uint16 tool_id = name ? TOOL_RENAME | SIMPLE_TOOL : 0;

	return call_tool_init(tool_id, (const char*)buf, 0, owner);
}


SQInteger command_work(HSQUIRRELVM vm)
{
	tool_t *tool = param<tool_t*>::get(vm, 1);

	player_t *player = param<player_t*>::get(vm, 2);

	koord3d pos1 = param<koord3d>::get(vm, 3);

	SQInteger top = sq_gettop(vm); // top >=3
	/* three possible calling conventions:
	 * (1) tool.work(pl, pos)
	 * (2) tool.work(pl, pos, param)
	 * (3) tool.work(pl, pos, pos2, param)
	 */
	const char* default_param = top>3 ? param<const char*>::get(vm, top) : NULL;
	koord3d     pos2          = top>4 ? param<koord3d>::get(vm, 4)       : koord3d::invalid;

	bool twoclick = top>4;

	// save & set default_param
	my_tool_t *mtool = get_attached_instance<my_tool_t>(vm, 1, param<tool_t*>::tag());
	if (mtool == NULL) {
		return sq_raise_error(vm, "Called from an instance different to tool_x");
	}
	plainstring &pdefault_param = mtool->default_param;
	pdefault_param = default_param; // copy
	tool->set_default_param( (const char*)pdefault_param ); // .. and set param

	if (!twoclick) {
		return param<call_tool_work>::push(vm, call_tool_work(tool, player, pos1));
	}
	else {
		return param<call_tool_work>::push(vm, call_tool_work(tool, player, pos1, pos2));
	}
}


// exec: the checks before a tool of the game's windows is called. The tools themselves fail
// silently (and some check the owner in network games only), so the failures that can be
// told beforehand are returned with a word: not_found, permission, invalid, funds.

static const char *const ERR_OWNER = "permission: the object belongs to another player";

uint8 get_flags(tool_t *tool);

/// the default_param after its first n fields
static const char *skip_fields(const char *p, int n)
{
	while (*p  &&  n > 0) {
		if (*p++ == ',') {
			n--;
		}
	}
	return p;
}

/// the convoy, line or halt of an id from a default_param, unbound if there is none (set_id does not check the range)
template<class T> static quickstone_tpl<T> handle_of(uint32 id)
{
	quickstone_tpl<T> h;
	if (id < quickstone_tpl<T>::get_size()) {
		h.set_id(id);
	}
	return h;
}

/// the player of this number, NULL if there is none
static player_t *player_of(sint64 nr)
{
	return nr >= 0  &&  nr < MAX_PLAYER_COUNT ? welt->get_player((uint8)nr) : NULL;
}

/**
 * See tool_change_convoi_t::init.
 * @param cost what the acting player pays (1/100 credits, negative), 0 if nothing
 */
static const char *exec_check_convoy(char cmd, const char *p, player_t *player, sint64 &cost)
{
	uint32 id = 0;
	if (sscanf(p, "%*c,%u", &id) != 1) {
		return "invalid: expected <command>,<convoy id>[,<value>]";
	}
	convoihandle_t cnv = handle_of<convoi_t>(id);
	if (!cnv.is_bound()) {
		return "not_found: no convoy with this id";
	}
	const char *rest = skip_fields(p, 2);
	switch (cmd) {
		case 'o': {
			// the player the convoy is offered to takes it over and pays for it (convoi_detail_t)
			player_t *buyer = cnv->get_permit_trade() ? player_of(cnv->get_accept_player_nr()) : NULL;
			if (buyer == NULL  ||  buyer == cnv->get_owner()) {
				return "invalid: the convoy is not offered to another player";
			}
			if (buyer != player) {
				return "permission: the convoy is offered to another player";
			}
			cost = -cnv->calc_restwert();
			return NULL;
		}
		case 's':
			// the schedule window sends this only (convoi_t::set_state accepts no others safely)
			if (atoi(rest) != convoi_t::EDIT_SCHEDULE) {
				return "invalid: the state can only be set to 1 (EDIT_SCHEDULE)";
			}
			break;
		case 'a':
			// offering (a cancels a running offer)
			if (!cnv->get_permit_trade()  &&  player_of(atoi(rest)) == NULL) {
				return "not_found: no player with this number";
			}
			break;
		case 'D':
		case 'Y': {
			sint16 x, y, z;
			if (sscanf(rest, "%hi,%hi,%hi", &x, &y, &z) != 3) {
				return "invalid: expected <command>,<convoy id>,<x>,<y>,<z> of the depot";
			}
			break;
		}
		case 'u':
			if (atoi(rest) != 0  &&  !cnv->is_suspended()  &&  !cnv->can_be_suspended()) {
				return "invalid: the convoy cannot be suspended now";
			}
			break;
	}
	if (!player_t::check_owner(cnv->get_owner(), player)) {
		return ERR_OWNER;
	}
	return NULL;
}

/// see tool_change_line_t::init
static const char *exec_check_line(char cmd, const char *p, player_t *player)
{
	uint32 id = 0;
	if (sscanf(p, "%*c,%u", &id) != 1) {
		return "invalid: expected <command>,<line id>[,<value>]";
	}
	if (strchr("Otu", cmd)) {
		// all lines (or convoys without a line) of the acting player
		return NULL;
	}
	linehandle_t line = handle_of<simline_t>(id);
	if (!line.is_bound()) {
		return "not_found: no line with this id";
	}
	if (!player_t::check_owner(line->get_owner(), player)) {
		return ERR_OWNER;
	}
	return NULL;
}

/// see tool_change_halt_t::init and tool_change_permission_t::init
static const char *exec_check_halt(const char *p, bool permission, player_t *player)
{
	// the tool toggles what haltestelle_t::is_allow_unload_longer_convoy returns, always false then
	if (!permission  &&  *p == 'l'  &&  !welt->get_settings().is_allow_unload_longer_convoy()) {
		return "invalid: the settings of the game do not allow unloading longer convoys";
	}
	uint32 id = 0, perms = 0;
	if (permission ? sscanf(p, "%u,%u", &id, &perms) != 2 : sscanf(p, "%*c,%u", &id) != 1) {
		return permission ? "invalid: expected <halt id>,<permission bits>" : "invalid: expected <command>,<halt id>";
	}
	if (perms > 0xFFFF) {
		return "invalid: the permission bits are 16 bits";
	}
	halthandle_t halt = handle_of<haltestelle_t>(id);
	if (!halt.is_bound()) {
		return "not_found: no halt with this id";
	}
	if (!player_t::check_owner(halt->get_owner(), player)) {
		return ERR_OWNER;
	}
	return NULL;
}

/// see tool_change_factory_t::init
static const char *exec_check_factory(char cmd, const char *p, player_t *player)
{
	koord pos;
	if (sscanf(p, "%*c,%hi,%hi", &pos.x, &pos.y) != 2) {
		return "invalid: expected <command>,<x>,<y>[,<value>]";
	}
	fabrik_t *fab = fabrik_t::get_fab(pos);
	if (fab == NULL) {
		return "not_found: no factory at this position";
	}
	if (!player_t::check_owner(fab->get_owner(), player)) {
		return ERR_OWNER;
	}
	// the limits of fabrik_info_t
	const int size = atoi(skip_fields(p, 3));
	if (cmd == 's'  &&  (size < 1  ||  size > 200)) {
		return "invalid: the shipment size is 1 to 200";
	}
	return NULL;
}

/// see tool_change_player_t::init
static const char *exec_check_player(char cmd, const char *p, player_t *player)
{
	int nr = -1, a = 0, b = 0;
	const int count = sscanf(p, "%*c,%i,%i,%i", &nr, &a, &b);
	if (cmd == 'n') {
		// see karte_t::change_player_tool
		if (count < 2) {
			return "invalid: expected n,<player>,<type>";
		}
		if (player != welt->get_public_player()) {
			return "permission: only the public player can create players";
		}
		if (nr < 2  ||  nr >= PLAYER_UNOWNED) {
			return "invalid: the player number is 2 to 14";
		}
		if (welt->get_player((uint8)nr)) {
			return "invalid: there is a player with this number";
		}
		if (a < player_t::HUMAN  ||  a >= player_t::MAX_AI) {
			return "invalid: the type is 1 (human), 2 (goods AI), 3 (passenger AI) or 4 (scripted AI)";
		}
		return NULL;
	}
	if (cmd == 'd') {
		if (count < 1) {
			return "invalid: expected d,<player>";
		}
		if (player_of(nr) == NULL  ||  nr >= PLAYER_UNOWNED) {
			return "not_found: no player with this number";
		}
		if (player_of(nr) == welt->get_public_player()) {
			return "invalid: the public player cannot be removed";
		}
		if (player != welt->get_public_player()  &&  player_of(nr) != player) {
			return "permission: only the public player or the player itself can remove a player";
		}
		return NULL;
	}
	if (count < (cmd == 'c' ? 3 : 2)) {
		return cmd == 'c' ? "invalid: expected c,<player>,<colour 1>,<colour 2>" : "invalid: expected $,<player>,<amount>";
	}
	if (player_of(nr) == NULL) {
		return "not_found: no player with this number";
	}
	if (cmd == 'c'  &&  player_of(nr) != player) {
		return "permission: only the player can change its colours";
	}
	if (cmd == '$'  &&  player != welt->get_public_player()) {
		return "permission: only the public player can change bank accounts";
	}
	return NULL;
}

/// see tool_recolour_t::init (it does not check the owner itself)
static const char *exec_check_recolour(const char *p, player_t *player)
{
	int nr = -1, colour = 0;
	if (sscanf(p + 1, "%i,%i", &nr, &colour) != 2) {
		return "invalid: expected <1 or 2><player>,<colour>";
	}
	player_t *target = player_of(nr);
	if (target == NULL) {
		return "not_found: no player with this number";
	}
	if (!player_t::check_owner(target, player)) {
		return ERR_OWNER;
	}
	return NULL;
}

/// see tool_merge_player_t::init
static const char *exec_check_merge(const char *p, player_t *player)
{
	int merged = -1, merger = -1;
	if (sscanf(p, "%i,%i", &merged, &merger) != 2) {
		return "invalid: expected <merged player>,<merging player>";
	}
	if (player != welt->get_public_player()) {
		return "permission: only the public player can merge players";
	}
	if (player_of(merged) == NULL  ||  player_of(merger) == NULL) {
		return "not_found: no player with this number";
	}
	if (merged == merger  ||  player_of(merged) == welt->get_public_player()  ||  player_of(merger) == welt->get_public_player()) {
		return "invalid: two different players other than the public player are merged";
	}
	return NULL;
}

/// see tool_change_traffic_light_t::init
static const char *exec_check_traffic_light(const char *p, player_t *player)
{
	sint16 x, y, z, which, ticks;
	if (sscanf(p, "%hi,%hi,%hi,%hi,%hi", &x, &y, &z, &which, &ticks) != 5) {
		return "invalid: expected <x>,<y>,<z>,<phase>,<ticks>";
	}
	const grund_t *gr = welt->lookup(koord3d(x, y, (sint8)z));
	roadsign_t *rs = gr ? gr->find<roadsign_t>() : NULL;
	if (rs == NULL) {
		return "not_found: no road sign at this position";
	}
	if (!player_t::check_owner(rs->get_owner(), player)) {
		return ERR_OWNER;
	}
	if (!rs->get_desc()->is_traffic_light()  &&  !rs->get_desc()->is_private_way()) {
		return "invalid: the sign is no traffic light or private way sign";
	}
	if (which < 0  ||  which > 4) {
		return "invalid: the phase is 0 to 4";
	}
	// the limits of trafficlight_info_t (the phases of a private way sign are the players allowed)
	const sint16 min_ticks = rs->get_desc()->is_traffic_light()  &&  which <= 1 ? 1 : 0;
	if (ticks < min_ticks  ||  ticks > 255) {
		return min_ticks ? "invalid: the ticks are 1 to 255" : "invalid: the ticks are 0 to 255";
	}
	return NULL;
}

/// see tool_change_roadsign_t::init
static const char *exec_check_roadsign(const char *p, player_t *player)
{
	sint16 x, y, z, value;
	char cmd = 0;
	if (sscanf(p, "%hi,%hi,%hi,%hi,%c", &x, &y, &z, &value, &cmd) != 5) {
		return "invalid: expected <x>,<y>,<z>,<value>,<command>";
	}
	if (strchr(*exec_tool_commands(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL), cmd) == NULL) {
		return "invalid: unknown command";
	}
	const grund_t *gr = welt->lookup(koord3d(x, y, (sint8)z));
	if (gr == NULL) {
		return "not_found: no road sign at this position";
	}
	// the tool looks for a signal or a road sign, depending on the command
	roadsign_t *rs;
	if (strchr("soamtdpw", cmd)) {
		rs = gr->find<signal_t>();
		if (rs == NULL) {
			return "not_found: no signal at this position";
		}
	}
	else {
		rs = cmd == 'l'  &&  gr->find<signal_t>() ? gr->find<signal_t>() : gr->find<roadsign_t>();
		if (rs == NULL) {
			return "not_found: no road sign at this position";
		}
	}
	if (!player_t::check_owner(rs->get_owner(), player)) {
		return ERR_OWNER;
	}
	const waytype_t wt = rs->get_waytype();
	switch (cmd) {
		case 'r':
			if (rs->get_intersection() == koord3d::invalid) {
				return "invalid: the sign is not in front of an intersection";
			}
			break;
		case 'c':
		case 'g':
		case 'm':
			if (wt == road_wt  ||  wt == water_wt  ||  wt == air_wt) {
				return "invalid: not for signs of this waytype";
			}
			break;
		case 'D':
			if (!rs->get_desc()->is_single_way()) {
				return "invalid: the sign is no one-way sign";
			}
			break;
		case 'n':
		case 'e':
			if (!rs->get_desc()->is_single_way()  ||  !rs->is_detailed_oneway()) {
				return "invalid: the sign is no detailed one-way sign";
			}
			break;
	}
	return NULL;
}

/// the commands of TOOL_SENDING_MONEY: the money goes to the player in the z of the position (money_frame_t)
static SQInteger exec_sending_money(HSQUIRRELVM vm, tool_t *tool, player_t *player, const char *p)
{
	player_t *receiver = NULL;
	if (sq_gettop(vm) >= 4) {
		receiver = sq_gettype(vm, 4) == OT_INTEGER ? player_of(param<sint64>::get(vm, 4)) : param<player_t*>::get(vm, 4);
	}
	if (receiver == NULL) {
		return sq_raise_error(vm, "not_found: no player to receive the money");
	}
	char *end = NULL;
	const sint64 amount = strtoll(p, &end, 10);
	if (end == p  ||  *end  ||  amount <= 0) {
		return sq_raise_error(vm, "invalid: expected a positive amount (1/100 credits)");
	}
	// the check of money_frame_t
	if (player != welt->get_public_player()  &&  player->get_finance()->get_account_balance() <= amount) {
		return sq_raise_error(vm, "funds: Insufficient funds!");
	}
	call_tool_work call(tool->get_id(), p, get_flags(tool), player, koord3d(0, 0, receiver->get_player_nr()));
	return param<call_tool_work>::push(vm, call);
}


SQInteger command_exec(HSQUIRRELVM vm)
{
	tool_t *tool = param<tool_t*>::get(vm, 1);
	player_t *pl = param<player_t*>::get(vm, 2);
	const char *p = param<const char*>::get(vm, 3);
	if (p == NULL) {
		p = "";
	}
	const uint16 id = tool->get_id();
	const char *const *commands = exec_tool_commands(id);
	if (commands == NULL) {
		return sq_raise_error(vm, "invalid: exec is for the tools of the game's windows, use work (tool 0x%x)", id);
	}
	// the acting player, as param<call_tool_init>::push takes it
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = pl;
	}
	if (player == NULL) {
		return sq_raise_error(vm, "invalid: no player");
	}
	if (id == (TOOL_SENDING_MONEY | SIMPLE_TOOL)) {
		return exec_sending_money(vm, tool, player, p);
	}

	// the command letter: the first one, the last field for road signs
	char cmd = id == (TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL) ? 0 : *p;
	if (*commands  &&  cmd  &&  strchr(*commands, cmd) == NULL) {
		return sq_raise_error(vm, "invalid: unknown command '%c'", cmd);
	}
	if (*commands  &&  cmd == 0  &&  id != (TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL)) {
		return sq_raise_error(vm, "invalid: no command");
	}

	sint64 cost = 0;
	const char *err = NULL;
	switch (id & 0xFFF) {
		case TOOL_CHANGE_CONVOI:        err = exec_check_convoy(cmd, p, player, cost); break;
		case TOOL_CHANGE_LINE:          err = exec_check_line(cmd, p, player); break;
		case TOOL_CHANGE_HALT:          err = exec_check_halt(p, false, player); break;
		case TOOL_HALT_PERMISSION:      err = exec_check_halt(p, true, player); break;
		case TOOL_CHANGE_FACTORY:       err = exec_check_factory(cmd, p, player); break;
		case TOOL_CHANGE_PLAYER:        err = exec_check_player(cmd, p, player); break;
		case TOOL_RECOLOUR_TOOL:        err = exec_check_recolour(p, player); break;
		case TOOL_MERGE_PLAYER:         err = exec_check_merge(p, player); break;
		case TOOL_CHANGE_TRAFFIC_LIGHT: err = exec_check_traffic_light(p, player); break;
		case TOOL_CHANGE_ROADSIGN:      err = exec_check_roadsign(p, player); break;
		case TOOL_UNDO:
			if (!player->can_undo()) {
				err = "invalid: nothing to undo";
			}
			break;
	}
	if (err) {
		return sq_raise_error(vm, "%s", err);
	}
	call_tool_init call(id, p, get_flags(tool), player);
	if (cost) {
		call.require_funds(player, cost);
	}
	return param<call_tool_init>::push(vm, call);
}

tool_t * call_tool_base_t::create_tool()
{
	tool_t *tool = ::create_tool(tool_id);
	if (tool) {
		tool->set_default_param(default_param);
		tool->flags = flags;
	}
	return tool;
}


SQInteger param<call_tool_work>::push(HSQUIRRELVM vm, call_tool_work v)
{
	if (v.error) {
		return sq_raise_error(vm, *v.error ? v.error : "Strange error occurred");
	}
	// create tool, if necessary, delete on exit
	std::unique_ptr<tool_t> our_tool;
	tool_t *tool = v.tool;
	if (tool == NULL) {
		our_tool.reset(v.create_tool());
		tool = our_tool.get();
	}
	if (tool == NULL) {
		return sq_raise_error(vm, "Called null tool");
	}
	// set correct flags
	tool->flags &= (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	tool->flags |= tool_t::WFL_SCRIPT;
	// get player parameter
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = v.player;
		// must be scenario - set flag
		tool->flags |= tool_t::WFL_NO_CHK;
	}
	// sanity check
	if (player == NULL) {
		return sq_raise_error(vm, "Called tool with player == null");
	}
	uint8 flags = tool->flags; // might be reset by init()

	// call init before work (but check network safety)
	if (!tool->is_init_network_safe()) {
		return sq_raise_error(vm, "Initializing tool has side effects");
	}
	if (!tool->init(player)) {
		return sq_raise_error(vm, "Error during initializing tool");
	}
	// set flags
	tool->flags = flags;
	// test work
	if (tool->is_work_network_safe()  ||  (!v.twoclick  &&  tool->is_work_here_network_safe(player, v.start))) {
		return sq_raise_error(vm, "Tool has no effects");
	}
	// two-click tool
	if (v.twoclick) {
		if (dynamic_cast<two_click_tool_t*>(tool)==NULL) {
			return sq_raise_error(vm, "Cannot call this tool with two coordinates");
		}
		if (!tool->is_work_here_network_safe(player, v.start)) {
			return sq_raise_error(vm, "First click has side effects");
		}
	}
	// check if calling suspendable tools is blocked
	if (const char* blocker = env_t::networkmode ? sq_get_suspend_blocker(vm) : NULL) {
		return sq_raise_error(vm, "busy: Cannot call this tool from within `%s'.", blocker);
	}

	bool suspended = false;
	const char* err = NULL;

	// first click of two_click_tool_t
	if (v.twoclick) {
		err = welt->call_work(tool, player, v.start, suspended);
		assert(!suspended);
	}
	// call the work that has effects
	if (err == NULL) {
		// register this tool call for callback with this id
		uint32 callback_id = suspended_scripts_t::get_unique_key(tool);
		tool->callback_id = callback_id;
		live_changes_t::tool_op_t op(player);
		err = welt->call_work(tool, player, v.twoclick ? v.end : v.start, suspended);

		if (suspended) {
			// register for wakeup
			suspended_scripts_t::register_suspended_script(callback_id, vm);
			// suspend vm for now, after wakeup it returns to the script
			return sq_suspendvm(vm);
		}
		if (script_vm_t::is_mcp(vm)) {
			op.done(vm, tool->get_id(), tool->get_default_param(), err);
		}
	}
	// return error message or NULL if tool was called
	return param<const char*>::push(vm, script_vm_t::with_error_word(vm, err));
}


uint8 get_flags(tool_t *tool)
{
	return tool->flags  & (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
}


void_t set_flags(tool_t *tool, uint8 flags)
{
	tool->flags = flags & (tool_t::WFL_SHIFT | tool_t:: WFL_CTRL);
	return void_t();
}


void* script_api::param<tool_t*>::tag()
{
	return (void*)param<tool_t*>::get;
}


const char* is_available(const obj_desc_timelined_t* desc)
{
	return desc->is_available(welt->get_timeline_year_month()) ? NULL : "Object not available (retired or future).";
}


/// the player of a preview, as param<call_tool_work>::push takes it
static player_t *preview_player(HSQUIRRELVM vm, player_t *pl)
{
	player_t *player = get_my_player(vm);
	return player ? player : pl;
}

/// pushes [x, y, z] (script coordinates), null for koord3d::invalid
static void push_pos(HSQUIRRELVM vm, koord3d pos)
{
	if (pos == koord3d::invalid) {
		sq_pushnull(vm);
		return;
	}
	koord k = pos.get_2d();
	coordinate_transform_t::koord_w2sq(k);
	const sint64 xyz[3] = { k.x, k.y, pos.z };
	sq_newarray(vm, 0);
	for (int i = 0; i < 3; i++) {
		param<sint64>::push(vm, xyz[i]);
		sq_arrayappend(vm, -2);
	}
}

/// a new tool with id, default_param and flags (scripted) for a preview or to be called by param<call_tool_work>::push
template<class T> static T *new_tool(uint16 id, const char *default_param, uint8 flags)
{
	T *tool = dynamic_cast<T*>(create_tool(id));
	assert(tool);
	tool->set_default_param(default_param);
	tool->flags = flags;
	return tool;
}

/// the spacing at @p index (if there), as wayobj_spacing_frame_t: 1 to 50
static const char *get_wayobj_spacing(HSQUIRRELVM vm, SQInteger index, uint8 &spacing)
{
	spacing = 1;
	if (index > sq_gettop(vm)  ||  sq_gettype(vm, index) == OT_NULL) {
		return NULL;
	}
	const sint64 v = param<sint64>::get(vm, index);
	if (v < 1  ||  v > 50) {
		return "invalid: the spacing is 1 to 50";
	}
	spacing = (uint8)v;
	return NULL;
}


struct build_way_args_t {
	bool keep_city_roads;
	sint8 overtaking_mode;
	uint8 street_flag;
	sint8 height_offset;
	sint8 vehicle_offset;

	build_way_args_t(bool keep_city_roads, sint8 overtaking_mode, uint8 street_flag, sint8 height_offset, sint8 vehicle_offset)
		: keep_city_roads(keep_city_roads), overtaking_mode(overtaking_mode), street_flag(street_flag), height_offset(height_offset), vehicle_offset(vehicle_offset) {}
};

/// the default_param of tool_build_way_t
static const char *build_way_param(const way_desc_t* way, const build_way_args_t &args)
{
	static cbuffer_t buf;
	buf.clear();
	buf.printf("%s,%d,%d,%d,%d", way->get_name(), args.overtaking_mode, args.street_flag, args.height_offset, args.vehicle_offset);
	return buf;
}

call_tool_work build_way(player_t* pl, koord3d start, koord3d end, const way_desc_t* way, bool straight, build_way_args_t args)
{
	if (way == NULL) {
		return call_tool_work("No way provided");
	}
	if (const char* err = is_available(way)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_WAY | GENERAL_TOOL, build_way_param(way, args), (straight ? 2 : 0) + (args.keep_city_roads ? 1 : 0), pl, start, end);
}


SQInteger command_build_way(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const way_desc_t* way = param<const way_desc_t*>::get(vm, 5);
	bool straight = param<bool>::get(vm, 6);

	bool keep_city_roads = false;
	sint8 overtaking_mode = 1 /*twoway_mode*/;
	uint8 street_flag = 0;
	sint8 height_offset = 0;
	sint8 vehicle_offset = 0;

	build_way_args_t args(keep_city_roads, overtaking_mode, street_flag, height_offset, vehicle_offset);
	return param<call_tool_work>::push(vm, build_way(pl, start, end, way, straight, args));
}

/// the arguments of build_road at @p index: a table, or keep_city_roads alone
static build_way_args_t get_build_way_args(HSQUIRRELVM vm, SQInteger index)
{
	bool keep_city_roads = false;
	sint8 overtaking_mode = 1 /*twoway_mode*/;
	uint8 street_flag = 0;
	sint8 height_offset = 0;
	sint8 vehicle_offset = 0;

	if (sq_gettype(vm, index) == OT_TABLE) {
		get_slot(vm, "keep_city_roads", keep_city_roads, index);
		sint32 mode = 1;
		if (SQ_SUCCEEDED(get_slot(vm, "overtaking_mode", mode, index))) {
			overtaking_mode = (sint8)mode;
		}
		sint32 flag = 0;
		if (SQ_SUCCEEDED(get_slot(vm, "street_flag", flag, index))) {
			street_flag = (uint8)flag;
		}
		sint32 ho = 0;
		if (SQ_SUCCEEDED(get_slot(vm, "height_offset", ho, index))) {
			height_offset = (sint8)ho;
		}
		sint32 vo = 0;
		if (SQ_SUCCEEDED(get_slot(vm, "vehicle_offset", vo, index))) {
			vehicle_offset = (sint8)vo;
		}
	}
	else {
		keep_city_roads = param<bool>::get(vm, index);
	}
	return build_way_args_t(keep_city_roads, overtaking_mode, street_flag, height_offset, vehicle_offset);
}

SQInteger command_build_road(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const way_desc_t* way = param<const way_desc_t*>::get(vm, 5);
	bool straight = param<bool>::get(vm, 6);

	return param<call_tool_work>::push(vm, build_way(pl, start, end, way, straight, get_build_way_args(vm, 7)));
}


/// no sync steps (INT_CHECK in the route searches) while a preview is calculated: the world stays as it is
struct intr_pause_t {
	bool was_enabled;
	intr_pause_t() : was_enabled(intr_is_enabled()) { intr_disable(); }
	~intr_pause_t() { if (was_enabled) { intr_enable(); } }
};

/// appends [x, y, z, extra...] (script coordinates) to the array on top of the stack
static void append_tile(HSQUIRRELVM vm, koord3d pos, const sint64 *extra, int n)
{
	koord k = pos.get_2d();
	coordinate_transform_t::koord_w2sq(k);
	const sint64 xyz[3] = { k.x, k.y, pos.z };
	sq_newarray(vm, 0);
	for (int i = 0; i < 3 + n; i++) {
		param<sint64>::push(vm, i < 3 ? xyz[i] : extra[i - 3]);
		sq_arrayappend(vm, -2);
	}
	sq_arrayappend(vm, -2);
}

static ribi_t::ribi ribi_to_script(ribi_t::ribi r)
{
	coordinate_transform_t::ribi_w2sq(r);
	return r;
}


SQInteger command_calc_way(HSQUIRRELVM vm)
{
	const uint32 t0 = dr_time();
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const way_desc_t* way = param<const way_desc_t*>::get(vm, 5);
	bool straight = param<bool>::get(vm, 6);
	build_way_args_t args = get_build_way_args(vm, 7);
	sint32 max_steps = 0;
	if (sq_gettype(vm, 7) == OT_TABLE) {
		get_slot(vm, "max_steps", max_steps, 7);
	}

	// the checks of build_way and param<call_tool_work>::push (raised there, so without the word of the tool errors)
	player_t *player = get_my_player(vm);
	const bool scenario_checks = player != NULL;
	if (player == NULL) {
		player = pl;
	}
	const char *err = NULL;
	bool tool_error = false;
	if (way == NULL) {
		err = "No way provided";
	}
	else if (const char *e = is_available(way)) {
		err = e;
	}
	else if (player == NULL) {
		err = "Called tool with player == null";
	}

	way_builder_t bauigel(player);
	bool reversed = false;
	if (err == NULL) {
		std::unique_ptr<tool_t> tool(create_tool(TOOL_BUILD_WAY | GENERAL_TOOL));
		tool->set_default_param(build_way_param(way, args));
		tool->flags = (straight ? tool_t::WFL_CTRL : 0) | (args.keep_city_roads ? tool_t::WFL_SHIFT : 0) | tool_t::WFL_SCRIPT;
		const uint8 flags = tool->flags;
		if (!tool->init(player)) {
			err = "Error during initializing tool";
		}
		tool->flags = flags;
		if (err == NULL) {
			tool_error = true;
			// the checks of the scenario at both clicks (karte_t::call_work)
			scenario_t *scen = welt->get_scenario();
			if (scenario_checks  &&  scen->is_scripted()) {
				if (!scen->is_tool_allowed(player, tool->get_id(), tool->get_waytype())) {
					err = "";
				}
				else if ((err = scen->is_work_allowed_here(player, tool->get_id(), tool->get_waytype(), start)) == NULL) {
					err = scen->is_work_allowed_here(player, tool->get_id(), tool->get_waytype(), end);
				}
			}
		}
		if (err == NULL) {
			bauigel.set_max_steps(max_steps > 0 ? max_steps : 0);
			bauigel.set_quiet(true);
			intr_pause_t pause;
			err = static_cast<tool_build_way_t*>(tool.get())->calc_preview_route(player, start, end, bauigel, reversed);
		}
	}
	if (err  &&  tool_error) {
		err = script_vm_t::with_error_word(vm, err);
	}

	vector_tpl<way_builder_t::preview_tile_t> tiles;
	vector_tpl<way_builder_t::preview_structure_t> structures;
	way_builder_t::cost_breakdown_t parts;
	sint64 estimate = 0;
	if (bauigel.get_count() > 0) {
		bauigel.calc_preview(tiles, structures, parts);
		estimate = bauigel.calc_costs();
	}

	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	create_slot<bool>(vm, "aborted", err != NULL  &&  bauigel.get_count() == 0  &&  bauigel.is_search_aborted());
	create_slot<bool>(vm, "reversed", reversed);
	create_slot<uint32>(vm, "length", bauigel.get_count());
	// from start to end
	sq_pushstring(vm, "tiles", -1);
	sq_newarray(vm, 0);
	for (uint32 i = 0; i < tiles.get_count(); i++) {
		const way_builder_t::preview_tile_t &t = tiles[reversed ? tiles.get_count() - 1 - i : i];
		const sint64 extra[2] = { ribi_to_script(t.ribi), t.flags };
		append_tile(vm, t.pos, extra, 2);
	}
	sq_newslot(vm, -3, false);
	for (int tunnel = 0; tunnel < 2; tunnel++) {
		sq_pushstring(vm, tunnel ? "tunnels" : "bridges", -1);
		sq_newarray(vm, 0);
		FOR(vector_tpl<way_builder_t::preview_structure_t>, const& st, structures) {
			if (st.tunnel == (tunnel != 0)) {
				koord k1 = st.start.get_2d(), k2 = st.end.get_2d();
				coordinate_transform_t::koord_w2sq(k1);
				coordinate_transform_t::koord_w2sq(k2);
				const sint64 v[6] = { k1.x, k1.y, st.start.z, k2.x, k2.y, st.end.z };
				sq_newarray(vm, 0);
				for (int i = 0; i < 6; i++) {
					param<sint64>::push(vm, v[i]);
					sq_arrayappend(vm, -2);
				}
				param<const char*>::push(vm, st.name);
				sq_arrayappend(vm, -2);
				sq_arrayappend(vm, -2);
			}
		}
		sq_newslot(vm, -3, false);
	}
	// what build_road books, and the estimate of the game's tool tip (it differs on some kept or replaced tiles)
	create_slot<sint64>(vm, "cost", parts.total());
	create_slot<sint64>(vm, "estimate", estimate);
	sq_pushstring(vm, "breakdown", -1);
	sq_newtable(vm);
	create_slot<sint64>(vm, "fresh", parts.fresh);
	create_slot<sint64>(vm, "replace", parts.replace);
	create_slot<sint64>(vm, "trees", parts.trees);
	create_slot<sint64>(vm, "groundobj", parts.groundobj);
	create_slot<sint64>(vm, "bridge", parts.bridge);
	create_slot<sint64>(vm, "tunnel", parts.tunnel);
	sq_newslot(vm, -3, false);
	// only rivers draw random numbers in the route search (way_builder_t::is_allowed_step), and the way tools build no rivers
	create_slot<bool>(vm, "random", false);
	create_slot<uint32>(vm, "ms", dr_time() - t0);
	return 1;
}


SQInteger command_calc_route_on_ways(HSQUIRRELVM vm)
{
	const uint32 t0 = dr_time();
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const waytype_t wt = param<waytype_t>::get(vm, 5);
	const uint32 flags = param<uint32>::get(vm, 6);
	const bool unmasked = flags & 1;

	// the waytype of tool_build_wayobj_t::calc_route and tool_build_roadsign_t::calc_route
	waytype_t search_wt = wt == tram_wt ? track_wt : wt;
	const char *err = NULL;
	grund_t *gr = welt->lookup(start);
	if (gr == NULL  ||  welt->lookup(end) == NULL) {
		err = "invalid: no ground at the start or the end";
	}
	else if (search_wt == any_wt) {
		if (weg_t *w = gr->get_weg(wt)) {
			search_wt = w->get_waytype();
		}
		else {
			err = "invalid: no way at the start";
		}
	}

	route_t route;
	bool blocked_by_masks = false;
	if (err == NULL) {
		intr_pause_t pause;
		bool ok = calc_route_on_ways(route, start, end, search_wt, unmasked);
		if (ok  &&  (flags & 2)  &&  start != end  &&  tool_build_roadsign_t::is_detour_of_neighbour(route, start, end, wt)) {
			ok = false;
		}
		if (!ok) {
			if (!unmasked) {
				route_t route_unmasked;
				blocked_by_masks = calc_route_on_ways(route_unmasked, start, end, search_wt, true);
			}
			route.clear();
			err = script_vm_t::with_error_word(vm, "Ways not connected");
		}
	}

	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	sq_pushstring(vm, "tiles", -1);
	sq_newarray(vm, 0);
	for (uint32 i = 0; i < route.get_count(); i++) {
		const sint64 extra[1] = { ribi_to_script(route.get_route().get_ribi(i)) };
		append_tile(vm, route.at(i), extra, 1);
	}
	sq_newslot(vm, -3, false);
	create_slot<bool>(vm, "blocked_by_masks", blocked_by_masks);
	create_slot<uint32>(vm, "ms", dr_time() - t0);
	return 1;
}


SQInteger command_build_wayobj(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const way_obj_desc_t* wayobj = param<const way_obj_desc_t*>::get(vm, 5);
	if (wayobj == NULL) {
		return sq_raise_error(vm, "No wayobj provided");
	}
	if (const char* err = is_available(wayobj)) {
		return sq_raise_error(vm, "%s", err);
	}
	uint8 spacing;
	if (const char *err = get_wayobj_spacing(vm, 6, spacing)) {
		return sq_raise_error(vm, "%s", err);
	}
	std::unique_ptr<tool_build_wayobj_t> tool(new_tool<tool_build_wayobj_t>(TOOL_BUILD_WAYOBJ | GENERAL_TOOL, wayobj->get_name(), 0));
	tool->set_spacing(spacing);
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
}


SQInteger command_build_wayobj_on_route(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	const way_obj_desc_t* wayobj = param<const way_obj_desc_t*>::get(vm, 4);
	const uint32 flags = param<uint32>::get(vm, 5);
	if (wayobj == NULL) {
		return sq_raise_error(vm, "No wayobj provided");
	}
	if (const char* err = is_available(wayobj)) {
		return sq_raise_error(vm, "%s", err);
	}
	// the tiles [[x, y, z, ...], ...], further entries (the ribi and flags of calc_way) are ignored
	const SQInteger count = sq_getsize(vm, 3);
	if (count < 1  ||  count > 2000) {
		return sq_raise_error(vm, "invalid: expected 1 to 2000 tiles");
	}
	vector_tpl<koord3d> tiles(count);
	for (SQInteger i = 0; i < count; i++) {
		sq_pushinteger(vm, i);
		sq_get(vm, 3);
		SQInteger v[3];
		bool ok = sq_gettype(vm, -1) == OT_ARRAY  &&  sq_getsize(vm, -1) >= 3;
		for (int j = 0; ok  &&  j < 3; j++) {
			sq_pushinteger(vm, j);
			ok = SQ_SUCCEEDED(sq_get(vm, -2))  &&  SQ_SUCCEEDED(sq_getinteger(vm, -1, &v[j]));
			sq_pop(vm, 1);
		}
		sq_pop(vm, 1);
		if (!ok) {
			return sq_raise_error(vm, "invalid: tile %d is no [x, y, z]", (int)i);
		}
		koord k((sint16)v[0], (sint16)v[1]);
		coordinate_transform_t::koord_sq2w(k);
		tiles.append(koord3d(k, (sint8)v[2]));
	}
	uint8 spacing;
	if (const char *err = get_wayobj_spacing(vm, 6, spacing)) {
		return sq_raise_error(vm, "%s", err);
	}
	static cbuffer_t buf;
	buf.clear();
	buf.printf("%u,", (flags & 1) | (spacing > 1 ? spacing << 1 : 0));
	if (!tool_build_wayobj_route_t::encode_route(buf, tiles)) {
		return sq_raise_error(vm, "invalid: the tiles are no neighbours one after another");
	}
	buf.printf(",%s", wayobj->get_name());
	// the check of tool_build_wayobj_t::is_valid_pos on every tile, the tool does it again (silently)
	player_t *player = get_my_player(vm);
	if (player == NULL) {
		player = pl;
	}
	for (uint32 i = 0; i < tiles.get_count(); i++) {
		if (!tool_build_wayobj_route_t::is_valid_tile(player, tiles[i], wayobj)) {
			return sq_raise_error(vm, "invalid: no way for %s of this player at tile %d (%s)", wayobj->get_name(), (int)i, tiles[i].get_str());
		}
	}
	return param<call_tool_work>::push(vm, call_tool_work(TOOL_BUILD_WAYOBJ_ROUTE | GENERAL_TOOL, buf, 0, pl, tiles[0]));
}


typedef call_tool_work(*bsr_type)(player_t*, koord3d, const building_desc_t*, my_ribi_t);

call_tool_work build_station_rotation(player_t* pl, koord3d pos, const building_desc_t* building, my_ribi_t rotation)
{
	// rotation: SENW -> 0123, see station_building_select_t
	int rot = -1;
	switch( (ribi_t::ribi)rotation )
	{
		case ribi_t::south: rot = 0; break;
		case ribi_t::east:  rot = 1; break;
		case ribi_t::north: rot = 2; break;
		case ribi_t::west:  rot = 3; break;
		default: ;
	}
	if (building == NULL  ||  !building->is_transport_building()) {
		return call_tool_work("No building provided");
	}
	if (const char* err = is_available(building)) {
		return call_tool_work(err);
	}
	static cbuffer_t buf;
	buf.clear();
	buf.printf("%s,%i", building->get_name(), rot);
	return call_tool_work(TOOL_BUILD_STATION | GENERAL_TOOL, buf, 0, pl, pos);
}

SQInteger command_build_station(HSQUIRRELVM vm)
{
	/* possible calling conventions:
	 *
	 * build_station(player, pos, desc)           - top == 4
	 * build_station(player, pos, desc, rotation) - top == 5
	 */
	if (sq_gettop(vm) == 4) {
		// rotation parameter missing, push default value
		sq_pushinteger(vm, ribi_t::all);
	}
	return embed_call_t<bsr_type>::call_function(vm, build_station_rotation, false);
}


call_tool_work build_depot(player_t* pl, koord3d pos, const building_desc_t* building)
{
	if (building == NULL  ||  !building->is_depot()) {
		return call_tool_work("No depot provided");
	}
	if (const char* err = is_available(building)) {
		return call_tool_work(err);
	}
	return call_tool_work(TOOL_BUILD_DEPOT | GENERAL_TOOL, building->get_name(), 0, pl, pos);
}

/// the settings of bridges and tunnels: the arguments of build_road, and high (bridges)
struct structure_args_t {
	sint8 overtaking_mode;
	uint8 street_flag;
	sint8 vehicle_offset;
	sint8 high; ///< -1: not given
	structure_args_t() : overtaking_mode(twoway_mode), street_flag(0), vehicle_offset(0), high(-1) {}
};

/// the arguments at @p index (if there): a table, a bool (high) or null; error if they do not fit
static const char *get_structure_args(HSQUIRRELVM vm, SQInteger index, structure_args_t &args)
{
	if (index > sq_gettop(vm)) {
		return NULL;
	}
	switch (sq_gettype(vm, index)) {
		case OT_NULL:
			return NULL;
		case OT_BOOL:
			args.high = param<bool>::get(vm, index) ? 1 : 0;
			return NULL;
		case OT_TABLE: {
			bool high = false;
			if (SQ_SUCCEEDED(get_slot(vm, "high", high, index))) {
				args.high = high ? 1 : 0;
			}
			sint32 mode = twoway_mode, flag = 0, vo = 0;
			get_slot(vm, "overtaking_mode", mode, index);
			get_slot(vm, "street_flag", flag, index);
			get_slot(vm, "vehicle_offset", vo, index);
			// the values of overtaking_mode_frame_t
			if (mode < halt_mode  ||  mode > inverted_mode) {
				return "invalid: the overtaking mode is -1 to 4";
			}
			if (flag < 0  ||  flag > 255  ||  vo < -128  ||  vo > 127) {
				return "invalid: the street flag is 0 to 255, the vehicle offset -128 to 127";
			}
			args.overtaking_mode = (sint8)mode;
			args.street_flag = (uint8)flag;
			args.vehicle_offset = (sint8)vo;
			return NULL;
		}
		default:
			return "invalid: the arguments are a table, a bool (high) or null";
	}
}

/// the bridge (last but one or last parameter) and whether an end (parameter 4) is given
static const bridge_desc_t *get_bridge_param(HSQUIRRELVM vm, SQInteger &desc_index, bool &with_end)
{
	// (pl, start, bridge [, args]) or (pl, start, end, bridge [, args]): the descriptor is an instance, args are not
	desc_index = sq_gettop(vm) >= 5  &&  sq_gettype(vm, 5) == OT_INSTANCE ? 5 : 4;
	with_end = desc_index == 5;
	return param<const bridge_desc_t*>::get(vm, desc_index);
}

SQInteger command_build_bridge(HSQUIRRELVM vm, bool with_end)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = with_end ? param<koord3d>::get(vm, 4) : koord3d::invalid;
	const SQInteger desc_index = with_end ? 5 : 4;
	const bridge_desc_t* bridge = param<const bridge_desc_t*>::get(vm, desc_index);
	if (bridge == NULL) {
		return sq_raise_error(vm, "No bridge provided");
	}
	if (const char* err = is_available(bridge)) {
		return sq_raise_error(vm, "%s", err);
	}
	structure_args_t args;
	if (const char *err = get_structure_args(vm, desc_index + 1, args)) {
		return sq_raise_error(vm, "%s", err);
	}
	// two clicks: ctrl for the first one to set the start only (the bridge is as high as possible then, if high is not given)
	std::unique_ptr<tool_build_bridge_t> tool(new_tool<tool_build_bridge_t>(TOOL_BUILD_BRIDGE | GENERAL_TOOL, bridge->get_name(), with_end ? tool_t::WFL_CTRL : 0));
	tool->set_script_values((overtaking_mode_t)args.overtaking_mode, args.street_flag, args.vehicle_offset, args.high);
	if (with_end) {
		return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
	}
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start));
}

SQInteger command_build_bridge_to(HSQUIRRELVM vm) { return command_build_bridge(vm, true); }
SQInteger command_build_bridge_at(HSQUIRRELVM vm) { return command_build_bridge(vm, false); }


SQInteger command_calc_bridge(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	SQInteger desc_index;
	bool with_end;
	const bridge_desc_t* bridge = get_bridge_param(vm, desc_index, with_end);
	koord3d end = with_end ? param<koord3d>::get(vm, 4) : koord3d::invalid;
	structure_args_t args;
	const char *err = get_structure_args(vm, desc_index + 1, args);
	if (err == NULL  &&  bridge == NULL) {
		err = "No bridge provided";
	}
	if (err == NULL) {
		err = is_available(bridge);
	}
	player_t *player = preview_player(vm, pl);
	if (err == NULL  &&  player == NULL) {
		err = "Called tool with player == null";
	}
	sint8 height = 0;
	sint64 cost = 0;
	if (err == NULL) {
		// as build_bridge(_at): the same flags and values
		std::unique_ptr<tool_build_bridge_t> tool(new_tool<tool_build_bridge_t>(TOOL_BUILD_BRIDGE | GENERAL_TOOL, bridge->get_name(), (with_end ? tool_t::WFL_CTRL : 0) | tool_t::WFL_SCRIPT));
		tool->set_script_values((overtaking_mode_t)args.overtaking_mode, args.street_flag, args.vehicle_offset, args.high);
		const uint8 flags = tool->flags;
		if (!tool->init(player)) {
			err = "Error during initializing tool";
		}
		else {
			tool->flags = flags;
			intr_pause_t pause;
			err = script_vm_t::with_error_word(vm, tool->calc_preview(player, start, end, height, cost));
		}
	}
	if (err) {
		end = koord3d::invalid;
	}
	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	sq_pushstring(vm, "end", -1);
	push_pos(vm, end);
	sq_newslot(vm, -3, false);
	create_slot<uint32>(vm, "length", err ? 0 : koord_distance(start, end) + 1);
	create_slot<sint32>(vm, "height", err ? 0 : height);
	create_slot<sint64>(vm, "cost", cost);
	return 1;
}


/// the tunnel and whether an end is given, as get_bridge_param
static const tunnel_desc_t *get_tunnel_param(HSQUIRRELVM vm, SQInteger &desc_index, bool &with_end)
{
	const SQInteger top = sq_gettop(vm);
	desc_index = top >= 5  &&  sq_gettype(vm, 5) == OT_INSTANCE ? 5 : 4;
	with_end = desc_index == 5;
	return param<const tunnel_desc_t*>::get(vm, desc_index);
}

SQInteger command_build_tunnel(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	SQInteger desc_index;
	bool with_end;
	const tunnel_desc_t* tunnel = get_tunnel_param(vm, desc_index, with_end);
	koord3d end = with_end ? param<koord3d>::get(vm, 4) : koord3d::invalid;
	if (tunnel == NULL) {
		return sq_raise_error(vm, "No tunnel provided");
	}
	if (const char* err = is_available(tunnel)) {
		return sq_raise_error(vm, "%s", err);
	}
	structure_args_t args;
	if (const char *err = get_structure_args(vm, desc_index + 1, args)) {
		return sq_raise_error(vm, "%s", err);
	}
	// one click at a slope: the tunnel through the hill (high: the portal only, as with ctrl);
	// two clicks from a tunnel: the way under ground to end
	// (ctrl: one click builds the portal only; two clicks would replace faster ways with it)
	std::unique_ptr<tool_build_tunnel_t> tool(new_tool<tool_build_tunnel_t>(TOOL_BUILD_TUNNEL | GENERAL_TOOL, tunnel->get_name(), args.high > 0  &&  !with_end ? tool_t::WFL_CTRL : 0));
	tool->set_script_values((overtaking_mode_t)args.overtaking_mode, args.street_flag, args.vehicle_offset);
	if (with_end) {
		return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
	}
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start));
}


SQInteger command_calc_tunnel(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	const tunnel_desc_t* tunnel = param<const tunnel_desc_t*>::get(vm, 4);
	structure_args_t args;
	const char *err = get_structure_args(vm, 5, args);
	if (err == NULL  &&  tunnel == NULL) {
		err = "No tunnel provided";
	}
	if (err == NULL) {
		err = is_available(tunnel);
	}
	player_t *player = preview_player(vm, pl);
	if (err == NULL  &&  player == NULL) {
		err = "Called tool with player == null";
	}
	koord3d end = koord3d::invalid;
	sint64 cost = 0;
	bool terraform = false;
	if (err == NULL) {
		std::unique_ptr<tool_build_tunnel_t> tool(new_tool<tool_build_tunnel_t>(TOOL_BUILD_TUNNEL | GENERAL_TOOL, tunnel->get_name(), (args.high > 0 ? tool_t::WFL_CTRL : 0) | tool_t::WFL_SCRIPT));
		const uint8 flags = tool->flags;
		if (!tool->init(player)) {
			err = "Error during initializing tool";
		}
		else {
			tool->flags = flags;
			intr_pause_t pause;
			err = script_vm_t::with_error_word(vm, tool->calc_preview(player, start, end, cost, terraform));
		}
	}
	if (err) {
		end = koord3d::invalid;
	}
	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	sq_pushstring(vm, "end", -1);
	push_pos(vm, end);
	sq_newslot(vm, -3, false);
	create_slot<uint32>(vm, "length", err ? 0 : koord_distance(start, end) + 1);
	create_slot<sint32>(vm, "height", 0);
	create_slot<sint64>(vm, "cost", cost);
	create_slot<bool>(vm, "terraform", terraform);
	return 1;
}


/// the settings of the window of the signal tool (signal_spacing_frame_t)
struct sign_args_t {
	uint8 spacing;
	bool remove_intermediate, replace_other, two_ways;
	sign_args_t() : spacing(2), remove_intermediate(true), replace_other(true), two_ways(false) {}
};

static const char *get_sign_args(HSQUIRRELVM vm, SQInteger index, sign_args_t &args)
{
	if (index > sq_gettop(vm)  ||  sq_gettype(vm, index) == OT_NULL) {
		return NULL;
	}
	if (sq_gettype(vm, index) != OT_TABLE) {
		return "invalid: the arguments are a table or null";
	}
	sint32 spacing = args.spacing;
	get_slot(vm, "spacing", spacing, index);
	get_slot(vm, "remove_intermediate", args.remove_intermediate, index);
	get_slot(vm, "replace_other", args.replace_other, index);
	get_slot(vm, "two_ways", args.two_ways, index);
	// (the window takes up to 255, but the tool counts twice the spacing in a byte)
	if (spacing < 1  ||  spacing > 127) {
		return "invalid: the spacing is 1 to 127";
	}
	args.spacing = (uint8)spacing;
	return NULL;
}

SQInteger command_build_sign_at(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d pos = param<koord3d>::get(vm, 3);
	const roadsign_desc_t* sign = param<const roadsign_desc_t*>::get(vm, 4);
	if (sign == NULL) {
		return sq_raise_error(vm, "No sign provided");
	}
	if (const char* err = is_available(sign)) {
		return sq_raise_error(vm, "%s", err);
	}
	ribi_t::ribi dir = ribi_t::none;
	if (sq_gettop(vm) >= 5  &&  sq_gettype(vm, 5) != OT_NULL) {
		dir = param<my_ribi_t>::get(vm, 5);
		// the directions the sign can have here (tool_build_roadsign_t::place_sign_intern)
		const grund_t *gr = welt->lookup(pos);
		const weg_t *weg = gr ? gr->get_weg(sign->get_wtyp() != tram_wt ? sign->get_wtyp() : track_wt) : NULL;
		if (weg == NULL) {
			return sq_raise_error(vm, "invalid: no way for this sign here");
		}
		const ribi_t::ribi way_dir = weg->get_ribi_unmasked();
		bool ok;
		if (sign->is_signal_type()) {
			ok = ribi_t::is_twoway(way_dir)  &&  (dir == way_dir  ||  (ribi_t::is_single(dir)  &&  (dir & way_dir) == dir));
		}
		else if (sign->is_single_way()  ||  sign->is_choose_sign()) {
			ok = dir != ribi_t::none  &&  (dir & ~way_dir) == 0;
		}
		else {
			ok = dir == way_dir;
		}
		if (!ok) {
			return sq_raise_error(vm, "invalid: the sign cannot have this direction here");
		}
	}
	std::unique_ptr<tool_build_roadsign_t> tool(new_tool<tool_build_roadsign_t>(TOOL_BUILD_ROADSIGN | GENERAL_TOOL, sign->get_name(), 0));
	if (dir) {
		sign_args_t args;
		tool->set_script_values(args.spacing, args.remove_intermediate, args.replace_other, args.two_ways, dir);
	}
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, pos));
}

/// build_signs_on_route and calc_signs_on_route
static SQInteger signs_on_route(HSQUIRRELVM vm, bool build)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const roadsign_desc_t* sign = param<const roadsign_desc_t*>::get(vm, 5);
	sign_args_t args;
	const char *err = get_sign_args(vm, 6, args);
	if (err == NULL  &&  sign == NULL) {
		err = "No sign provided";
	}
	if (err == NULL) {
		err = is_available(sign);
	}
	if (build) {
		if (err) {
			return sq_raise_error(vm, "%s", err);
		}
		// two clicks: ctrl for the first one to set the start only (it would place a sign otherwise)
		std::unique_ptr<tool_build_roadsign_t> tool(new_tool<tool_build_roadsign_t>(TOOL_BUILD_ROADSIGN | GENERAL_TOOL, sign->get_name(), tool_t::WFL_CTRL));
		tool->set_script_values(args.spacing, args.remove_intermediate, args.replace_other, args.two_ways, ribi_t::none);
		return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
	}

	player_t *player = preview_player(vm, pl);
	if (err == NULL  &&  player == NULL) {
		err = "Called tool with player == null";
	}
	vector_tpl<tool_build_roadsign_t::sign_step_t> steps;
	sint64 cost = 0;
	if (err == NULL) {
		std::unique_ptr<tool_build_roadsign_t> tool(new_tool<tool_build_roadsign_t>(TOOL_BUILD_ROADSIGN | GENERAL_TOOL, sign->get_name(), tool_t::WFL_CTRL | tool_t::WFL_SCRIPT));
		tool->set_script_values(args.spacing, args.remove_intermediate, args.replace_other, args.two_ways, ribi_t::none);
		const uint8 flags = tool->flags;
		if (!tool->init(player)) {
			err = "Error during initializing tool";
		}
		else {
			tool->flags = flags;
			route_t route;
			intr_pause_t pause;
			if (!tool->calc_sign_route(route, player, start, end)) {
				err = script_vm_t::with_error_word(vm, "Ways not connected");
			}
			else {
				tool->plan_signs(player, route, steps, cost);
			}
		}
	}
	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	sq_pushstring(vm, "tiles", -1);
	sq_newarray(vm, 0);
	FOR(vector_tpl<tool_build_roadsign_t::sign_step_t>, const& st, steps) {
		if (st.dir) {
			const sint64 extra[1] = { ribi_to_script(st.dir) };
			append_tile(vm, st.pos, extra, 1);
		}
	}
	sq_newslot(vm, -3, false);
	sq_pushstring(vm, "remove", -1);
	sq_newarray(vm, 0);
	FOR(vector_tpl<tool_build_roadsign_t::sign_step_t>, const& st, steps) {
		if (!st.dir) {
			append_tile(vm, st.pos, NULL, 0);
		}
	}
	sq_newslot(vm, -3, false);
	create_slot<sint64>(vm, "cost", cost);
	return 1;
}

SQInteger command_build_signs_on_route(HSQUIRRELVM vm) { return signs_on_route(vm, true); }
SQInteger command_calc_signs_on_route(HSQUIRRELVM vm) { return signs_on_route(vm, false); }


SQInteger command_change_way_settings(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const sint64 mode = param<sint64>::get(vm, 5);
	const sint64 flag = sq_gettop(vm) >= 6 ? param<sint64>::get(vm, 6) : 0;
	if (mode < halt_mode  ||  mode > inverted_mode) {
		return sq_raise_error(vm, "invalid: the overtaking mode is -1 to 4");
	}
	if (flag < 0  ||  flag > 255) {
		return sq_raise_error(vm, "invalid: the street flag is 0 to 255");
	}
	std::unique_ptr<tool_change_way_settings_t> tool(new_tool<tool_change_way_settings_t>(TOOL_CHANGE_WAY_SETTINGS | GENERAL_TOOL, "", 0));
	tool->set_overtaking_mode((overtaking_mode_t)mode);
	tool->set_street_flag((uint8)flag);
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
}

SQInteger command_change_way_offset(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const sint64 vo = param<sint64>::get(vm, 5);
	if (vo < -128  ||  vo > 127) {
		return sq_raise_error(vm, "invalid: the vehicle offset is -128 to 127");
	}
	// the waytype of the first click: given, else the first way there
	static cbuffer_t buf;
	buf.clear();
	if (sq_gettop(vm) >= 6  &&  sq_gettype(vm, 6) != OT_NULL) {
		buf.printf("%d", (int)param<waytype_t>::get(vm, 6));
	}
	std::unique_ptr<tool_change_way_offset_t> tool(new_tool<tool_change_way_offset_t>(TOOL_CHANGE_WAY_OFFSET | GENERAL_TOOL, buf, 0));
	tool->set_packed_vehicle_offset((sint8)vo);
	return param<call_tool_work>::push(vm, call_tool_work(tool.get(), pl, start, end));
}


SQInteger command_calc_remove(HSQUIRRELVM vm)
{
	player_t* pl = param<player_t*>::get(vm, 2);
	koord3d start = param<koord3d>::get(vm, 3);
	koord3d end = param<koord3d>::get(vm, 4);
	const char *kind = param<const char*>::get(vm, 5);
	waytype_t wt = sq_gettop(vm) >= 6  &&  sq_gettype(vm, 6) != OT_NULL ? param<waytype_t>::get(vm, 6) : invalid_wt;
	player_t *player = preview_player(vm, pl);

	const char *err = NULL;
	route_t route;
	// the tiles and why they are refused, as the tools check them
	vector_tpl<koord3d> tiles;
	vector_tpl<sint64> remaining;
	vector_tpl<koord3d> refused;
	vector_tpl<const char*> reasons;
	intr_pause_t pause;
	if (player == NULL) {
		err = "Called tool with player == null";
	}
	else if (welt->lookup(start) == NULL  ||  welt->lookup(end) == NULL) {
		err = "invalid: no ground at the start or the end";
	}
	else if (kind  &&  strcmp(kind, "way") == 0) {
		// tool_wayremover_t: is_valid_pos at both clicks, calc_route, do_work
		static cbuffer_t buf;
		buf.clear();
		buf.printf("%d", (int)wt);
		std::unique_ptr<tool_wayremover_t> tool(new_tool<tool_wayremover_t>(TOOL_WAYREMOVER | GENERAL_TOOL, buf, tool_t::WFL_SCRIPT));
		const waytype_t route_wt = wt == tram_wt ? track_wt : wt;
		const grund_t *gs = welt->lookup(start), *ge = welt->lookup(end);
		if (wt == invalid_wt  ||  wt == any_wt) {
			err = "invalid: a waytype is needed";
		}
		else if ((wt == powerline_wt ? gs->get_leitung() == NULL : gs->get_weg(route_wt) == NULL)  ||  (wt == powerline_wt ? ge->get_leitung() == NULL : ge->get_weg(route_wt) == NULL)) {
			err = "invalid: no way of this waytype at the start or the end";
		}
		else if (gs->get_depot()  ||  ge->get_depot()) {
			err = script_vm_t::with_error_word(vm, "No suitable ground!");
		}
		else if (!tool->find_route(route, player, start, end)) {
			err = script_vm_t::with_error_word(vm, "Ways not connected");
		}
		else {
			for (uint32 i = 0; i < route.get_count(); i++) {
				const grund_t *gr = welt->lookup(route.at(i));
				if (const char *e = tool_wayremover_t::check_tile(gr, wt, player)) {
					refused.append(route.at(i));
					reasons.append(*e ? e : "Ways not connected");
				}
				else {
					// the directions that stay (do_work): none if start = end
					const ribi_t::ribi rem = route.get_count() <= 1 ? 0 : 15 ^ route.get_route().get_ribi(i);
					const ribi_t::ribi have = wt == powerline_wt ? gr->get_leitung()->get_ribi() : gr->get_weg_ribi_unmasked(route_wt);
					tiles.append(route.at(i));
					remaining.append(ribi_to_script(have & rem));
				}
			}
		}
	}
	else if (kind  &&  strcmp(kind, "wayobj") == 0) {
		// tool_remove_wayobj_t (tool_build_wayobj_t without build): calc_route_on_ways
		const grund_t *gs = welt->lookup(start);
		if (wt == invalid_wt) {
			err = "invalid: a waytype is needed";
		}
		else if (gs->get_weg(wt) == NULL) {
			err = "invalid: no way of this waytype at the start";
		}
		else {
			waytype_t route_wt = wt == tram_wt ? track_wt : wt;
			if (route_wt == any_wt) {
				route_wt = gs->get_weg(wt)->get_waytype();
			}
			if (!calc_route_on_ways(route, start, end, route_wt)) {
				err = script_vm_t::with_error_word(vm, "Ways not connected");
			}
			else {
				for (uint32 i = 0; i < route.get_count(); i++) {
					const grund_t *gr = welt->lookup(route.at(i));
					for (uint8 n = 0; n < gr->get_top(); n++) {
						wayobj_t *wo = obj_cast<wayobj_t>(gr->obj_bei(n));
						if (wo  &&  wo->get_waytype() == wt) {
							if (const char *e = wo->is_deletable(player)) {
								refused.append(route.at(i));
								reasons.append(*e ? e : "the tool failed");
							}
							else {
								tiles.append(route.at(i));
								remaining.append(0);
							}
							break;
						}
					}
				}
			}
		}
	}
	else if (kind  &&  strcmp(kind, "signal") == 0) {
		// tool_remove_signal_t: the waytype of the signal at the start (wt is not used), signals only (not signs)
		std::unique_ptr<tool_remove_signal_t> tool(new_tool<tool_remove_signal_t>(TOOL_REMOVE_SIGNAL | GENERAL_TOOL, "", tool_t::WFL_SCRIPT));
		if (!tool->calc_route(route, player, start, end)) {
			err = script_vm_t::with_error_word(vm, "Ways not connected");
		}
		else {
			for (uint32 i = 0; i < route.get_count(); i++) {
				const grund_t *gr = welt->lookup(route.at(i));
				if (signal_t *sig = gr ? gr->find<signal_t>() : NULL) {
					if (const char *e = sig->is_deletable(player)) {
						refused.append(route.at(i));
						reasons.append(*e ? e : "the tool failed");
					}
					else {
						tiles.append(route.at(i));
						remaining.append(0);
					}
				}
			}
		}
	}
	else {
		err = "invalid: kind is way, wayobj or signal";
	}
	const bool way = kind  &&  strcmp(kind, "way") == 0;
	if (err == NULL  &&  !refused.empty()  &&  way) {
		// the way remover refuses everything then
		err = script_vm_t::with_error_word(vm, reasons[0]);
	}

	sq_newtable(vm);
	create_slot<bool>(vm, "ok", err == NULL);
	create_slot<const char*>(vm, "error", err);
	sq_pushstring(vm, "tiles", -1);
	sq_newarray(vm, 0);
	for (uint32 i = 0; i < tiles.get_count(); i++) {
		const sint64 extra[1] = { remaining[i] };
		append_tile(vm, tiles[i], extra, way ? 1 : 0);
	}
	sq_newslot(vm, -3, false);
	sq_pushstring(vm, "refused", -1);
	sq_newarray(vm, 0);
	for (uint32 i = 0; i < refused.get_count(); i++) {
		koord k = refused[i].get_2d();
		coordinate_transform_t::koord_w2sq(k);
		sq_newarray(vm, 0);
		param<sint64>::push(vm, k.x);
		sq_arrayappend(vm, -2);
		param<sint64>::push(vm, k.y);
		sq_arrayappend(vm, -2);
		param<sint64>::push(vm, refused[i].z);
		sq_arrayappend(vm, -2);
		// the reason, with the word of the tool errors
		param<const char*>::push(vm, script_vm_t::with_error_word(vm, reasons[i]));
		sq_arrayappend(vm, -2);
		sq_arrayappend(vm, -2);
	}
	sq_newslot(vm, -3, false);
	return 1;
}

call_tool_work set_slope(player_t* pl, koord3d start, my_slope_t slope)
{
	// communicate per default_param
	static char buf[8];
	sprintf(buf, "%2d", (uint8)slope);
	static tool_setslope_t tool;
	// we do not want our slopes translated to double-height (even for single-height paksets), they are already in the double-height system
	tool.old_slope_compatibility_mode = false;
	tool.set_default_param(buf);
	return call_tool_work(&tool, pl, start);
}

call_tool_work restore_slope(player_t* pl, koord3d start)
{
	return call_tool_work(TOOL_RESTORESLOPE | GENERAL_TOOL, "", 0, pl, start);
}

const char* can_set_slope(player_t* pl, koord3d pos, my_slope_t slope)
{
	return tool_setslope_t::tool_set_slope_work(pl, pos, slope, false /* compatibility */, true /* check */);
}

sint64 set_slope_get_price(my_slope_t slope)
{
	return slope == RESTORE_SLOPE ? -welt->get_settings().cst_alter_land : -welt->get_settings().cst_set_slope;
}

call_tool_init set_signal_stop_before_check(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,t", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_start_signal(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,p", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_advance_to_end(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,a", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_guide_signal(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,s", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_choose_signal(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,o", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_skip_default_route(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,d", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_length_based(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,l", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_margin_length(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,m", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_init set_signal_two_ways(player_t* pl, koord3d pos, int val)
{
	cbuffer_t buf;
	buf.printf("%hi,%hi,%hi,%hi,w", (sint16)pos.x, (sint16)pos.y, (sint16)pos.z, (sint16)val);
	return call_tool_init(TOOL_CHANGE_ROADSIGN | SIMPLE_TOOL, (const char*)buf, 0, pl);
}

call_tool_work change_climate_at(player_t* pl, koord3d start, int climate)
{
	if (climate < water_climate || climate >= MAX_CLIMATES) {
		return call_tool_work("Invalid climate number provided");
	}
	// communicate per default_param
	static cbuffer_t param;
	param.clear();
	param.printf("%d", climate);
	return call_tool_work(TOOL_SET_CLIMATE | GENERAL_TOOL, param, 0, pl, start, start);
}


call_tool_work lower_raise(player_t* pl, koord3d pos, bool lower)
{
	// need to transform coordinate to grid coordinates
	switch(coordinate_transform_t::get_rotation()) {
		case 1: pos += koord(1,0); break;
		case 2: pos += koord(1,1); break;
		case 3: pos += koord(0,1); break;
		default:;
	}
	return call_tool_work((lower ? TOOL_LOWER_LAND : TOOL_RAISE_LAND) | GENERAL_TOOL, NULL, 0, pl, pos);
}

void export_commands(HSQUIRRELVM vm)
{
	/**
	 * Proof-of-concept to make tools available to scripts.
	 *
	 * The default_param is not checked. Use at own risk.
	 * @ingroup game_cmd
	 */
	create_class(vm, "command_x");

	/**
	 * Constructor to obtain a tool.
	 * Accepts the general tools and the simple tools of the game's windows that change the game
	 * (tool_change_convoi ... tool_change_roadsign, see exec), but no tools of the screen or the
	 * progress of the game.
	 * @param id id of the tool
	 * @typemask command_x(tool_ids)
	 */
	register_function(vm, command_constructor, "constructor", 2, "xi");
	// set type tag to custom getter
	sq_settypetag(vm, -1, param<tool_t*>::tag());

	/**
	 * @returns flags set for this tool
	 */
	register_method(vm, &get_flags, "get_flags", true);
	/**
	 * Sets flags for this tool.
	 * Simulates pressing shift or ctrl while clicking with mouse.
	 * @param flags bitmap, 1 = shift pressed, 2 = ctrl pressed
	 */
	register_method(vm, &set_flags, "set_flags", true);
	/**
	 * Does the dirty work.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param pos coordinate, where something should happen
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d)
	 */
	register_function(vm, command_work, "work", -3, "x t|x|y t|x|y");
	/**
	 * Calls a tool of the game's windows (the simple tools of tool_ids, see the constructor) with
	 * the parameter string the windows send to it (its default_param). Failures that can be told
	 * beforehand raise an error starting with a word: not_found (no such convoy, line, halt, factory,
	 * player or sign), permission (another player's object), invalid (the parameter string or the
	 * state does not fit), funds.
	 *
	 * The tools and their commands (first letter of @p param; the last field for tool_change_roadsign):
	 * - tool_change_convoi "<command>,<convoy id>[,<value>]": n s d y D Y r e t c m b i u k a o
	 *   ('o': the player the convoy is offered to takes it over)
	 * - tool_change_line "<command>,<line id>[,<value>]": p m o O t u w
	 * - tool_change_halt "<command>,<halt id>": c p m w l
	 * - tool_halt_permission "<halt id>,<permission bits>"
	 * - tool_change_factory "<command>,<x>,<y>[,<value>]": d s
	 * - tool_change_player "<command>,<player>,...": c $ n d ("n,<player>,<type>" creates a player,
	 *   the public player only; "d,<player>" removes it with all it has, the public player or the player itself)
	 * - tool_recolour "<1|2><player>,<colour>"
	 * - tool_sending_money "<amount>" (1/100 credits), with the receiving player as third parameter
	 * - tool_merge_player "<merged player>,<merging player>" (the public player only)
	 * - tool_undo ""
	 * - tool_change_traffic_light "<x>,<y>,<z>,<phase>,<ticks>"
	 * - tool_change_roadsign "<x>,<y>,<z>,<value>,<command>": r c g D n e w s o a t d p l m
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to call the tool as
	 * @param param the parameter string
	 * @param receiver (tool_sending_money only) the player receiving the money, or its number
	 * @returns null upon success (tool_sending_money: an error string otherwise)
	 * @typemask void(player_x,string)
	 */
	register_function(vm, command_exec, "exec", -3, "x t|x|y s|o .");

#ifdef DOXYGEN
	// command_work works with different numbers of parameters.
	// Their documentation is included here.

	/**
	 * Does the dirty work.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param pos coordinate, where something should happen
	 * @param param magic parameter string
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d,string)
	 */
	register_function(vm,, "work");
	const char* work(player_x player, coord3d pos, string param);
	/**
	 * Does the dirty work.
	 * Needs two positions for start and end of the work, for e.g., road building.
	 * @note In network games script will be suspended until the command is executed.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param param magic parameter string
	 * @returns null upon success, an error string otherwise
	 * @typemask string(player_x,coord3d,coord3d,string)
	 */
	register_function(vm,, "work");
#endif
	/**
	 * Build a way.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param way type of way to be built
	 * @param straight force building of straight ways, similar as building way with control key pressed
	 */
	STATIC register_function(vm, command_build_way, "build_way", 6 /* exactly 6 parameters */, ".xt|x|yt|x|yt|x|yb", true /* static */);
	/**
	 * Build a road.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param way type of way to be built
	 * @param straight force building of straight ways, similar as building way with control key pressed
	 * @param args table of arguments (keep_city_roads, overtaking_mode, street_flag, height_offset, vehicle_offset)
	 */
	STATIC register_function(vm, command_build_road, "build_road", 7, ".xt|x|yt|x|yt|x|yb.", true /* static */);
	/**
	 * What build_road with the same arguments would build now, without changing anything (a preview).
	 * The checks, the route and the costs are those of the game's way tool.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param way type of way to be built
	 * @param straight as build_road (control key)
	 * @param args as build_road, and max_steps: the most steps of the route search (default and
	 *        upper limit: max_route_steps of the settings)
	 * @returns table {ok, error (as build_road would return it, or null), aborted (the search ran out
	 *          of steps), reversed (the route was searched from the end), length, ms,
	 *          tiles: [[x, y, z, ribi, flags], ...] from start to end (ribi: existing | route directions;
	 *          flags: 1 new, 2 replaced, 4 kept, 8 crossing, 32 trees removed, 64 bridge, 128 tunnel,
	 *          256 city road kept, 512 the owner changes), bridges, tunnels: [[x1, y1, z1, x2, y2, z2, name], ...]
	 *          added by the way builder, cost (what the building books, in 1/100 credits, positive: costs),
	 *          breakdown of cost {fresh, replace, trees, groundobj, bridge, tunnel}, estimate (the
	 *          estimate of the game's tool tip, way_builder_t::calc_costs), random (false)}
	 * @typemask table(player_x,coord3d,coord3d,way_desc_x,bool,table)
	 */
	STATIC register_function(vm, command_calc_way, "calc_way", 7, ".xt|x|yt|x|yt|x|yb.", true /* static */);
	/**
	 * The route over existing ways that the way object and signal tools search between two tiles
	 * (steps of equal cost), without changing anything.
	 * @param pl player (not used)
	 * @param start coordinate, where the route begins
	 * @param end   coordinate, where the route ends
	 * @param waytype waytype of the ways (tram: track, any: the way at the start)
	 * @param flags 1: ignore the directions closed by signs (but the route never enters a tile
	 *        against its one-way sign), 2: fail as the signal tool does if start and end are
	 *        neighbours but the route goes around
	 * @returns table {ok, error, tiles: [[x, y, z, ribi], ...], blocked_by_masks (no route because of
	 *          closed directions, without flag 1), ms}
	 * @typemask table(player_x,coord3d,coord3d,way_types,int)
	 */
	STATIC register_function(vm, command_calc_route_on_ways, "calc_route_on_ways", 6, ".xt|x|yt|x|yii", true /* static */);
	/**
	 * Build a depot.
	 * @param pl player to pay for the work
	 * @param pos position to place the depot
	 * @param depot type of depot to be built
	 */
	STATIC register_method(vm, build_depot, "build_depot", false, true);
	/**
	 * Build a station or station extension building.
	 * @param pl player to pay for the work
	 * @param pos position to place the depot
	 * @param station type of station to be built
	 * @param rotaton (optional parameter) rotation of building (only used for flat docks, put direction from land to water here)
	 */
	STATIC register_function(vm, command_build_station, "build_station", -4 /* at least 4 parameters */,
							 func_signature_t<bsr_type>::get_typemask(false).c_str(), false /* static */);

	log_squirrel_type(func_signature_t<bsr_type>::get_squirrel_class(false), "build_station", func_signature_t<bsr_type>::get_squirrel_type(false, 0));
	/**
	 * Build a bridge.
	 * Similar to drag-and-build of bridges in-game.
	 * @param pl player to pay for the work
	 * @param start coordinate, where bridge begins
	 * @param end   coordinate, where bridge ends
	 * @param bridge type of bridge to be built
	 * @param args (optional) table {high, overtaking_mode, street_flag, vehicle_offset} (as build_road), or
	 *        high alone. high: true the highest bridge to this end (as with ctrl in the game), false the lowest.
	 *        Without it, the bridge is the highest (as before).
	 * @typemask string(player_x,coord3d,coord3d,bridge_desc_x,table)
	 */
	STATIC register_function(vm, command_build_bridge_to, "build_bridge", -5, ".xt|x|yt|x|yt|x|y", true /* static */);
	/**
	 * Build a bridge.
	 * Similar to one click with mouse on suitable start tile: program will figure out bridge span itself.
	 * @param pl player to pay for the work
	 * @param start coordinate, where bridge begins, the end point will be automatically determined
	 * @param bridge type of bridge to be built
	 */
	STATIC register_function(vm, command_build_bridge_at, "build_bridge_at", -4, ".xt|x|yt|x|y", true /* static */);
	/**
	 * Where and how build_bridge (with end) or build_bridge_at (without) would build a bridge now,
	 * without changing anything.
	 * @param pl player to pay for the work
	 * @param start coordinate, where bridge begins
	 * @param end (optional) coordinate, where bridge ends
	 * @param bridge type of bridge
	 * @param args as build_bridge
	 * @returns table {ok, error, end [x, y, z], length (tiles from start to end), height (above start),
	 *          cost (what building books, in 1/100 credits, positive; without a way that the game adds
	 *          to connect a single way at the ends)}
	 * @typemask table(player_x,coord3d,coord3d,bridge_desc_x,table)
	 */
	STATIC register_function(vm, command_calc_bridge, "calc_bridge", -4, ".xt|x|yt|x|y", true /* static */);
	/**
	 * Build a tunnel.
	 * One click at a slope: the tunnel through the hill (args.high: only the portal, as with ctrl; not used with end).
	 * Two clicks from a tunnel tile: the tunnel under ground to end.
	 * Fails with the reason if there can be no tunnel.
	 * @param pl player to pay for the work
	 * @param start coordinate, where the tunnel begins
	 * @param end (optional) coordinate, where the tunnel ends
	 * @param tunnel type of tunnel
	 * @param args (optional) table {high, overtaking_mode, street_flag, vehicle_offset} (as build_road)
	 * @typemask string(player_x,coord3d,coord3d,tunnel_desc_x,table)
	 */
	STATIC register_function(vm, command_build_tunnel, "build_tunnel", -4, ".xt|x|yt|x|y", true /* static */);
	/**
	 * Where build_tunnel (one click) would build a tunnel now, without changing anything.
	 * @param pl player to pay for the work
	 * @param start coordinate, where the tunnel begins
	 * @param tunnel type of tunnel
	 * @param args (optional) as build_tunnel
	 * @returns table {ok, error, end [x, y, z], length, height (0), cost (1/100 credits, positive, without
	 *          changing the ground at the end), terraform (the ground at the end is changed)}
	 * @typemask table(player_x,coord3d,tunnel_desc_x,table)
	 */
	STATIC register_function(vm, command_calc_tunnel, "calc_tunnel", -4, ".xt|x|yt|x|y", true /* static */);
	/**
	 * Modify the slope of one tile.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 * @param slope new slope, can also be one of @ref slope::all_up_slope or @ref slope::all_down_slope.
	 */
	STATIC register_method(vm, set_slope, "set_slope", false, true);
	/**
	 * Restore natural slope of one tile.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 */
	STATIC register_method(vm, restore_slope, "restore_slope", false, true);
	/**
	 * Checks whether player @p pl can do this terraforming.
	 * @param pl player
	 * @param pos position
	 * @param slope new slope, can also be one of @ref slope::all_up_slope or @ref slope::all_down_slope
	 * @returns null (if allowed) or an error message otherwise
	 */
	STATIC register_method(vm, can_set_slope, "can_set_slope", false, true);
	/**
	 * Costs of using @ref set_slope.
	 * @returns cost
	 */
	STATIC register_method(vm, set_slope_get_price, "slope_get_price", false, true);
	/**
	 * Build signal / road-sign. If such a sign already exists then change its direction.
	 * @param pl player to pay for the work
	 * @param pos position of tile
	 * @param sign type of road-sign or signal to be built
	 * @param dir (optional) the direction of the sign (sign_x::get_dir): signals one direction of the way or both
	 *        (the way's directions), one-way and choose signs some directions of the way, other signs the
	 *        directions of the way. Without it, an existing sign turns.
	 * @typemask string(player_x,coord3d,sign_desc_x,dir)
	 */
	STATIC register_function(vm, command_build_sign_at, "build_sign_at", -4, ".xt|x|yt|x|y", true /* static */);
	/**
	 * Build signals / road-signs on the route between start and end, as dragging the game's sign tool.
	 * @param pl player to pay for the work
	 * @param start coordinate, where the route begins
	 * @param end coordinate, where the route ends
	 * @param sign type of road-sign or signal to be built
	 * @param args (optional) table {spacing (1 to 127, default 2), remove_intermediate (default true),
	 *        replace_other (default true), two_ways (default false)}, the settings of the tool's window
	 * @returns null upon success, an error string otherwise (calc_signs_on_route tells the places)
	 * @typemask string(player_x,coord3d,coord3d,sign_desc_x,table)
	 */
	STATIC register_function(vm, command_build_signs_on_route, "build_signs_on_route", -5, ".xt|x|yt|x|yt|x|y", true /* static */);
	/**
	 * Where build_signs_on_route would place and remove signs now, without changing anything.
	 * @returns table {ok, error, tiles: [[x, y, z, dir], ...] the signs placed (dir as sign_x::get_dir),
	 *          remove: [[x, y, z], ...] the signs removed between, cost (the estimate of the tool, 1/100 credits)}
	 * @typemask table(player_x,coord3d,coord3d,sign_desc_x,table)
	 */
	STATIC register_function(vm, command_calc_signs_on_route, "calc_signs_on_route", -5, ".xt|x|yt|x|yt|x|y", true /* static */);
	/**
	 * Set or clear the "stop before check" flag on a rail signal.
	 * Applicable to simple signals, longblock signals and choose signals.
	 * @param pl  player who owns the signal
	 * @param pos position of the signal tile
	 * @param val 1 to enable, 0 to disable
	 * @returns null on success, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_stop_before_check, "set_stop_before_check", false, true);
	/**
	 * Set or clear the "start signal" flag on a rail signal.
	 * When set, a convoy in CAN_START or CAN_START_ONE_MONTH state checks
	 * whether this signal (the next signal ahead) is clear before departing.
	 * If the signal is RED the convoy stays put; if GREEN it departs normally.
	 * @param pl  player who owns the signal
	 * @param pos position of the signal tile
	 * @param val 1 to enable, 0 to disable
	 * @returns null on success, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_start_signal, "set_start_signal", false, true);
	/**
	 * Sets "advance to end" flag on a choose signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new state of the flag
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_advance_to_end, "set_advance_to_end", false, true);
	/**
	 * Sets "require parent convoy to enter" (guide signal) flag on a signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new state of the flag
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_guide_signal, "set_guide_signal", false, true);
	/**
	 * Sets "choose signal" flag on a signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new state of the flag
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_choose_signal, "set_choose_signal", false, true);
	/**
	 * Sets "skip default route" flag on a choose signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new state of the flag
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_skip_default_route, "set_skip_default_route", false, true);
	/**
	 * Sets "length based" flag on a choose signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new state of the flag
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_length_based, "set_length_based", false, true);
	/**
	 * Sets "margin length" on a choose signal.
	 * @param pl player executing the command
	 * @param pos position of signal
	 * @param val new margin length
	 * @returns null if successful, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_margin_length, "set_margin_length", false, true);
	/**
	 * Set or clear the "allow reverse passage" (two ways) flag of a signal.
	 * When set (true), trains coming from the reverse direction are allowed to pass the signal.
	 * When cleared (false), the signal strictly blocks trains from the reverse direction.
	 * @param pl  player who owns the signal
	 * @param pos position of the signal tile
	 * @param val 1 to enable, 0 to disable
	 * @returns null on success, an error string otherwise
	 * @typemask string(player_x, coord3d, int)
	 */
	STATIC register_method(vm, set_signal_two_ways, "set_two_ways", false, true);
	/**
	 * Build way-object.
	 * @param pl player to pay for the work
	 * @param start coordinate, where work begins
	 * @param end   coordinate, where work ends
	 * @param wayobj type of wayobj to be built
	 * @param spacing (optional) on every n-th tile only (1 to 50, default 1), as in the tool's window
	 * @typemask string(player_x,coord3d,coord3d,way_obj_desc_x,int)
	 */
	STATIC register_function(vm, command_build_wayobj, "build_wayobj", -5, ".xt|x|yt|x|yt|x|y", true /* static */);
	/**
	 * Build way-object on the given tiles, without a route search (unlike build_wayobj).
	 * @param pl player to pay for the work
	 * @param tiles [[x, y, z], ...] neighbours one after another with ways for the way object, at most 2000
	 * @param wayobj type of wayobj to be built
	 * @param flags 1: replace faster way objects too (control key)
	 * @param spacing (optional) on every n-th of the tiles only (1 to 50, default 1)
	 * @typemask string(player_x,array,way_obj_desc_x,int,int)
	 */
	STATIC register_function(vm, command_build_wayobj_on_route, "build_wayobj_on_route", -5, ".xat|x|yi", true /* static */);
	/**
	 * Change the overtaking mode and the street flags of the roads on the route between start and end
	 * (the game's tool, tool_change_way_settings). One-way modes (0, -1) make them one-way from start to end
	 * (street_x::get_oneway_ribi), the others two-way. Roads of other players stay as they are.
	 * @param pl player to pay for the work
	 * @param start coordinate, where the route begins
	 * @param end coordinate, where the route ends
	 * @param overtaking_mode -1 to 4 (as build_road)
	 * @param street_flag (optional) as build_road, default 0
	 * @typemask string(player_x,coord3d,coord3d,int,int)
	 */
	STATIC register_function(vm, command_change_way_settings, "change_way_settings", -5, ".xt|x|yt|x|yi", true /* static */);
	/**
	 * Change the vehicle offset of the ways on the route between start and end (tool_change_way_offset).
	 * @param pl player to pay for the work
	 * @param start coordinate, where the route begins
	 * @param end coordinate, where the route ends
	 * @param vehicle_offset as build_road
	 * @param waytype (optional) the ways to change, default the first way at start
	 * @typemask string(player_x,coord3d,coord3d,int,way_types)
	 */
	STATIC register_function(vm, command_change_way_offset, "change_way_offset", -5, ".xt|x|yt|x|yi", true /* static */);
	/**
	 * What the removing tools would remove between start and end now, without changing anything.
	 * @param pl player to pay for the work
	 * @param start coordinate, where the route begins
	 * @param end coordinate, where the route ends
	 * @param kind "way" (tool_remove_way), "wayobj" (tool_remove_wayobj) or "signal" (tool_remove_signal, signals only)
	 * @param waytype the waytype of the ways or way objects (not used for signals)
	 * @returns table {ok, error, tiles: [[x, y, z(, ribi)], ...] the tiles where things are removed (way: ribi
	 *          the directions that stay, 0 the way goes), refused: [[x, y, z, reason], ...]}. A way is removed
	 *          only if no tile is refused; way objects and signals are removed on the other tiles.
	 * @typemask table(player_x,coord3d,coord3d,string,way_types)
	 */
	STATIC register_function(vm, command_calc_remove, "calc_remove", -5, ".xt|x|yt|x|ys", true /* static */);
	/**
	 * Change climate of tile
	 * @param pl player to pay for the work
	 * @param pos coordinate of tile
	 * @param climate new climate, possible values see @ref climates
	 */
	STATIC register_method(vm, change_climate_at, "change_climate_at", false, true);

	/**
	 * Lower grid point
	 * @param pl player to pay for the work
	 * @param pos coordinate of tile, grid point in nw corner will be lowered
	 */
	STATIC register_method_fv(vm, lower_raise, "grid_lower", freevariable<bool>(true), false, true);
	/**
	 * Raise grid point
	 * @param pl player to pay for the work
	 * @param pos coordinate of tile, grid point in nw corner will be lowered
	 */
	STATIC register_method_fv(vm, lower_raise, "grid_raise", freevariable<bool>(false), false, true);

	end_class(vm);
}
