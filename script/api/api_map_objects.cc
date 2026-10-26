/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "api.h"

/** @file api_map_objects.cc exports all map-objects. */

#include "api_obj_desc_base.h"
#include "api_simple.h"
#include "../api_class.h"
#include "../api_function.h"

#include "../../obj/simobj.h"
#include "../../simdepot.h"
#include "../../simworld.h"
#include "../../boden/grund.h"
#include "../../dataobj/scenario.h"
#include "../../descriptor/ground_desc.h"
#include "../../obj/baum.h"
#include "../../obj/gebaeude.h"
#include "../../obj/field.h"
#include "../../obj/label.h"
#include "../../obj/leitung2.h"
#include "../../obj/roadsign.h"
#include "../../obj/signal.h"
#include "../../obj/wayobj.h"
#include "../../obj/bruecke.h"
#include "../../obj/tunnel.h"
#include "../../boden/wege/strasse.h"
#include "../../player/simplay.h"

// for depot tools
#include "../../simconvoi.h"
#include "../../dataobj/schedule.h"
#include "../../simmenu.h"
#include "../../descriptor/building_desc.h"
#include "../../descriptor/vehicle_desc.h"
#include "../../bauer/vehikelbauer.h"
#include "../../dataobj/route.h"
#include "../../vehicle/simvehicle.h"
#include "../../utils/simstring.h"

#include <stdarg.h>


using namespace script_api;

// use pointers to obj_t_tag[ <type> ] to tag the obj_t classes
static uint8 obj_t_tag[256];

/*
 * template struct to bind obj_t::typ to obj_t classes
 */
template<class D> struct bind_code;

/*
 * Class to templatify access of obj_t objects
 */
template<class D> struct access_objs {

	/*
	 * Access object: check whether object is still on this tile.
	 */
	static D* get_by_pos(HSQUIRRELVM vm, SQInteger index)
	{
		SQUserPointer tag = obj_t_tag + bind_code<D>::objtype;
		SQUserPointer p = NULL;
		if (SQ_SUCCEEDED(sq_getinstanceup(vm, index, &p, tag))  &&  p) {
			D *obj = static_cast<D*>(p);
			koord3d pos = param<koord3d>::get(vm, index);
			grund_t *gr = welt->lookup(pos);
			if (gr  &&  gr->obj_ist_da(obj)) {
				return obj;
			}
			else {
				// object or tile disappeared: clear userpointer
				sq_setinstanceup(vm, index, NULL);
			}
			sq_raise_error(vm, "Object of type %s vanished from (%s).", param<D*>::squirrel_type(), pos.get_str());
		}
		else {
			sq_raise_error(vm, "Object is not of type %s.", param<D*>::squirrel_type);
		}
		return NULL;
	}

	/*
	 * Create instance: call constructor with coordinates.
	 */
	static SQInteger push_with_pos(HSQUIRRELVM vm, D* const& obj)
	{
		if (obj == NULL) {
			sq_pushnull(vm);
			return 1;
		}
		koord pos = obj->get_pos().get_2d();
		coordinate_transform_t::koord_w2sq(pos);
		sint16 x = pos.x;
		sint16 y = pos.y;
		sint8  z = obj->get_pos().z;
		if (bind_code<D>::objtype == obj_t::obj) {
			// generic object, object type as fourth parameter
			if (!SQ_SUCCEEDED(push_instance(vm, script_api::param<D*>::squirrel_type(), x, y, z, obj->get_typ()))) {
				return SQ_ERROR;
			}
		}
		else if (bind_code<D>::objtype != obj->get_typ()) {
			// obj is instance of derived class
			return script_api::param<obj_t*>::push(vm, obj);
		}
		else {
			// specific object with its own constructor, type already preset as default parameter
			if (!SQ_SUCCEEDED(push_instance(vm, script_api::param<D*>::squirrel_type(), x, y, z))) {
				return SQ_ERROR;
			}
		}
		sq_setinstanceup(vm, -1, obj);
		return 1;
	}

};


SQInteger exp_obj_pos_constructor(HSQUIRRELVM vm) // parameters: sint16 x, sint16 y, sint8 z, obj_t::typ type
{
	// get coordinates
	sint16 x = param<sint16>::get(vm, 2);
	sint16 y = param<sint16>::get(vm, 3);
	sint8  z = param<sint16>::get(vm, 4);
	// set coordinates
	set_slot(vm, "x", x, 1);
	set_slot(vm, "y", y, 1);
	koord pos(x,y);
	coordinate_transform_t::koord_sq2w(pos);
	// find tile - some objects have larger z-coordinate (e.g., on foundations)
	grund_t *gr = welt->lookup(koord3d(pos, z));
	for(uint8 i=1, end = ground_desc_t::double_grounds ? 2 : 1; gr == NULL  &&  i<=end; i++) {
		gr = welt->lookup(koord3d(pos, z-i));
	}
	// find object and set instance up
	if (gr) {
		// correct z-coordinate
		set_slot(vm, "z", gr->get_pos().z, 1);
		// search for object
		obj_t::typ type = (obj_t::typ)param<uint8>::get(vm, 5);
		obj_t *obj = NULL;
		if (type == obj_t::roadsign  ||  type == obj_t::signal) {
			// special treatment of signals/roadsigns
			obj = gr->suche_obj(obj_t::roadsign);
			if (obj == NULL) {
				obj = gr->suche_obj(obj_t::signal);
			}
		}
		else if (type == obj_t::pumpe  ||  type == obj_t::senke) {
			obj = gr->suche_obj(obj_t::pumpe);
			if (obj == NULL) {
				obj = gr->suche_obj(obj_t::senke);
			}
		}
		else if (type != obj_t::old_airdepot) { // special treatment of depots
			obj = gr->suche_obj(type);
		}
		else {
			obj = gr->get_depot();
		}
		if (obj) {
			sq_setinstanceup(vm, 1, obj);
			return SQ_OK;
		}
	}
	return sq_raise_error(vm, "No object of requested type on tile (or no tile at this position)");
}

template<> struct bind_code<obj_t> { static const uint8 objtype = obj_t::obj; };

// macro to implement get and push for obj_t's with position
#define getpush_obj_pos(D, type) \
	D* script_api::param<D*>::get(HSQUIRRELVM vm, SQInteger index) \
	{ \
		return access_objs<D>::get_by_pos(vm, index); \
	} \
	SQInteger script_api::param<D*>::push(HSQUIRRELVM vm, D* const& obj) \
	{ \
		return access_objs<D>::push_with_pos(vm, obj); \
	} \
	template<> struct bind_code<D> { static const uint8 objtype = type; };

// implementation of get and push by macros
getpush_obj_pos(baum_t, obj_t::baum);
getpush_obj_pos(gebaeude_t, obj_t::gebaeude);
getpush_obj_pos(label_t, obj_t::label);
getpush_obj_pos(weg_t, obj_t::way);
getpush_obj_pos(strasse_t, obj_t::way);
getpush_obj_pos(leitung_t, obj_t::leitung);
getpush_obj_pos(field_t, obj_t::field);
getpush_obj_pos(wayobj_t, obj_t::wayobj);

namespace script_api {
	// each depot has its own class
	declare_specialized_param(depot_t*, "t|x|y", "depot_x");
	declare_specialized_param(airdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(narrowgaugedepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(bahndepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(strassendepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(schiffdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(monoraildepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(tramdepot_t*, "t|x|y", "depot_x");
	declare_specialized_param(maglevdepot_t*, "t|x|y", "depot_x");

	// map roadsign_t and signal_t to the same class
	declare_specialized_param(roadsign_t*, "t|x|y", "sign_x");
	declare_specialized_param(signal_t*, "t|x|y", "sign_x");

	//  map all to one transformer class
	declare_specialized_param(pumpe_t*,  "t|x|y", "transformer_x");
	declare_specialized_param(senke_t*,  "t|x|y", "transformer_x");

	// bridges and tunnels
	declare_specialized_param(bruecke_t*, "t|x|y", "bridge_x");
	declare_specialized_param(tunnel_t*,  "t|x|y", "tunnel_x");
};

// base depot class, use old_airdepot as identifier here
getpush_obj_pos(depot_t, obj_t::old_airdepot);
// now all the derived classes
getpush_obj_pos(airdepot_t, obj_t::airdepot);
getpush_obj_pos(narrowgaugedepot_t, obj_t::narrowgaugedepot);
getpush_obj_pos(bahndepot_t, obj_t::bahndepot);
getpush_obj_pos(strassendepot_t, obj_t::strassendepot);
getpush_obj_pos(schiffdepot_t, obj_t::schiffdepot);
getpush_obj_pos(monoraildepot_t, obj_t::monoraildepot);
getpush_obj_pos(tramdepot_t, obj_t::tramdepot);
getpush_obj_pos(maglevdepot_t, obj_t::maglevdepot);
// roadsigns/signals
getpush_obj_pos(roadsign_t, obj_t::roadsign);
getpush_obj_pos(signal_t, obj_t::signal);
//  powerlines
getpush_obj_pos(pumpe_t, obj_t::pumpe);
getpush_obj_pos(senke_t, obj_t::senke);
// bridges and tunnels
getpush_obj_pos(bruecke_t, obj_t::bruecke);
getpush_obj_pos(tunnel_t, obj_t::tunnel);


#define case_resolve_obj(D) \
	case bind_code<D>::objtype: \
		return script_api::param<D*>::push(vm, (D*)obj);

// we have to resolve instances of derived classes here...
SQInteger script_api::param<obj_t*>::push(HSQUIRRELVM vm, obj_t* const& obj)
{
	if (obj == NULL) {
		sq_pushnull(vm);
		return 1;
	}
	obj_t::typ type = obj->get_typ();
	switch(type) {
		case_resolve_obj(baum_t);
		case_resolve_obj(gebaeude_t);
		case_resolve_obj(label_t);
		case bind_code<weg_t>::objtype:
		if (((weg_t*)obj)->get_waytype() == road_wt) {
			return script_api::param<strasse_t*>::push(vm, (strasse_t*)obj);
		}
		return script_api::param<weg_t*>::push(vm, (weg_t*)obj);

		case_resolve_obj(roadsign_t);
		case_resolve_obj(signal_t);
		case_resolve_obj(field_t);

		case_resolve_obj(airdepot_t);
		case_resolve_obj(narrowgaugedepot_t);
		case_resolve_obj(bahndepot_t);
		case_resolve_obj(strassendepot_t);
		case_resolve_obj(schiffdepot_t);
		case_resolve_obj(monoraildepot_t);
		case_resolve_obj(tramdepot_t);
		case_resolve_obj(maglevdepot_t);

		case_resolve_obj(leitung_t);
		case_resolve_obj(pumpe_t);
		case_resolve_obj(senke_t);

		case_resolve_obj(wayobj_t);
		case_resolve_obj(bruecke_t);
		case_resolve_obj(tunnel_t);
		default:
			return access_objs<obj_t>::push_with_pos(vm, obj);
	}
}

obj_t* script_api::param<obj_t*>::get(HSQUIRRELVM vm, SQInteger index)
{
	return access_objs<obj_t>::get_by_pos(vm, index);
}

// return way ribis, have to implement a wrapper, to correctly rotate ribi
static SQInteger get_way_ribi(HSQUIRRELVM vm)
{
	weg_t *w = param<weg_t*>::get(vm, 1);
	bool masked = param<bool>::get(vm, 2);

	ribi_t::ribi ribi = w ? (masked ? w->get_ribi() : w->get_ribi_unmasked() ) : 0;

	return param<my_ribi_t>::push(vm, ribi);
}

static SQInteger get_overtaking_mode(HSQUIRRELVM vm)
{
	strasse_t *w = param<strasse_t*>::get(vm, 1);
	if (w) {
		return param<sint8>::push(vm, w->get_overtaking_mode());
	}
	return param<sint8>::push(vm, twoway_mode);
}

static SQInteger get_street_flags(HSQUIRRELVM vm)
{
	strasse_t *w = param<strasse_t*>::get(vm, 1);
	if (w) {
		return param<uint8>::push(vm, w->get_street_flag());
	}
	return param<uint8>::push(vm, 0);
}

// create class
template<class D>
void begin_obj_class(HSQUIRRELVM vm, const char* name, const char* base = NULL)
{
	SQInteger res = create_class(vm, name, base);
	if(  !SQ_SUCCEEDED(res)  ) {
		// base class not found: maybe script_base.nut is not up-to-date
		dbg->error( "begin_obj_class()", "Create class failed for %s. Base class %s missing. Please update simutrans (or just script/script_base.nut)!", name, base );
		sq_raise_error(vm, "Create class failed for %s. Base class %s missing. Please update simutrans (or just script/script_base.nut)!", name, base);
	}
	uint8 objtype = bind_code<D>::objtype;
	// store typetag to identify pointers
	sq_settypetag(vm, -1, obj_t_tag + objtype);
	// export constructor
	register_function_fv(vm, exp_obj_pos_constructor, "constructor", 4, "xiiii", freevariable<uint8>(objtype));
	// now functions can be registered
}

// mark objects
void_t mark_object(obj_t* obj)
{
	obj->set_flag(obj_t::highlight);
	obj->set_flag(obj_t::dirty);
	return void_t();
}
void_t unmark_object(obj_t* obj)
{
	obj->clear_flag(obj_t::highlight);
	obj->set_flag(obj_t::dirty);
	return void_t();
}
bool object_is_marked(obj_t* obj)
{
	return obj->get_flag(obj_t::highlight);
}

// markers / labels
call_tool_work create_marker(koord pos, player_t* player, const char* text)
{
	if (text == NULL) {
		return "Cannot create label with text == null";
	}
	return call_tool_work(TOOL_MARKER | GENERAL_TOOL, text, 0, player, koord3d(pos, 0));
}

call_tool_init label_set_text(label_t *l, const char* text)
{
	return command_rename(l->get_owner(), 'm', l->get_pos(), text);
}

const char* label_get_text(label_t* l)
{
	if (l) {
		if (grund_t *gr = welt->lookup(l->get_pos())) {
			return gr->get_text();
		}
	}
	return NULL;
}

// roadsign
bool roadsign_can_pass(const roadsign_t* rs, player_t* player)
{
	return player  &&  rs->get_desc()->is_private_way()  ?  (rs->get_player_mask() & (1<<player->get_player_nr()))!=0 : true;
}

sint32 roadsign_get_state(roadsign_t* rs)
{
	return (sint32)rs->get_state();
}

// depot
// Failures are raised with an error starting with a fixed word: permission, not_found, invalid, busy, funds

/// the convoy is in this depot (as the depot tool needs it)
static bool convoy_in_depot(const depot_t *depot, convoihandle_t cnv)
{
	return cnv->in_depot()  &&  cnv->get_home_depot() == depot->get_pos();
}

/// an error text with values; valid until the next call
static const char *depot_error(const char *format, ...)
{
	static char buf[1024];
	char text[512];
	va_list ap;
	va_start(ap, format);
	vsnprintf(text, sizeof(text), format, ap);
	va_end(ap);
	// the error is used as a format again (sq_raise_error): names may contain '%'
	char *d = buf;
	for (const char *c = text;  *c;  c++) {
		if (*c == '%') {
			*d++ = '%';
		}
		*d++ = *c;
	}
	*d = 0;
	return buf;
}

/// the call of the depot tool, see depot_t::call_depot_tool for the command string
static call_tool_init depot_tool(depot_t *depot, player_t *player, char tool, convoihandle_t cnv, const char *extra)
{
	cbuffer_t buf;
	buf.printf( "%c,%s,%u", tool, depot->get_pos().get_str(), cnv.get_id());
	if (extra) {
		buf.append(",");
		buf.append(extra);
	}
	call_tool_init call(TOOL_CHANGE_DEPOT | SIMPLE_TOOL, buf, 0, player);
	call.require_owner(depot->get_owner());
	return call;
}

/// number of vehicles of this type stored in the depot
static uint32 depot_stock(depot_t *depot, const vehicle_desc_t *desc)
{
	uint32 n = 0;
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		if (v->get_desc() == desc) {
			n++;
		}
	}
	return n;
}

/**
 * The vehicles the depot tool takes together with @p desc (tool_change_depot_t::init):
 * the unique leaders in front (insert) and the unique trailers behind (append).
 */
static void depot_composition(const vehicle_desc_t *desc, bool leaders, bool trailers, vector_tpl<const vehicle_desc_t*> &list)
{
	list.clear();
	const vehicle_desc_t *info = desc;
	if (leaders) {
		while (info->get_leader_count() == 1  &&  info->get_leader(0) != NULL  &&  info->get_leader(0) != vehicle_desc_t::any_vehicle  &&  !list.is_contained(info->get_leader(0))) {
			info = info->get_leader(0);
			list.insert_at(0, info);
		}
		info = desc;
	}
	list.append(info);
	if (trailers) {
		while (info->get_trailer_count() == 1  &&  info->get_trailer(0) != NULL  &&  info->get_trailer(0) != vehicle_desc_t::any_vehicle  &&  !list.is_contained(info->get_trailer(0))) {
			info = info->get_trailer(0);
			list.append(info);
		}
	}
}

/// the price of the vehicles of @p list that are not in the stock of the depot (1/100 credits)
static sint64 depot_purchase_cost(depot_t *depot, const vector_tpl<const vehicle_desc_t*> &list)
{
	sint64 cost = 0;
	vector_tpl<const vehicle_desc_t*> used;
	FOR(vector_tpl<const vehicle_desc_t*>, const desc, list) {
		uint32 n = 0;
		FOR(vector_tpl<const vehicle_desc_t*>, const u, used) {
			n += (u == desc);
		}
		if (n < depot_stock(depot, desc)) {
			used.append(desc);
		}
		else {
			cost += desc->get_price();
		}
	}
	return cost;
}

/// the depot tool buys this vehicle (it fails silently else): of an obsolete type only stored ones
static bool depot_can_buy(depot_t *depot, const vehicle_desc_t *desc)
{
	return desc->is_available(welt->get_timeline_year_month())  ||  welt->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot_stock(depot, desc) > 0;
}

/// common part of append_vehicle and insert_vehicle (tools 'a' and 'i')
static call_tool_init depot_add_vehicle(depot_t *depot, player_t *player, convoihandle_t cnv, const vehicle_desc_t *desc, bool infront)
{
	if (desc == NULL) {
		return "invalid: Invalid vehicle_desc_x provided";
	}
	// the tool ignores these silently (tool_change_depot_t::init)
	if (!depot->can_accept_waytype(desc->get_waytype())) {
		return "invalid: the vehicle is for another waytype";
	}
	if (cnv.is_bound()  &&  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	// vehicles that have to be together are bought together (only one for an invalid convoy)
	const bool is_invalid = cnv.is_bound()  &&  cnv->is_invalid_convoy();
	vector_tpl<const vehicle_desc_t*> list;
	depot_composition(desc, infront  &&  !is_invalid, !infront  &&  !is_invalid, list);
	if (!depot_can_buy(depot, list.front())) {
		return "invalid: Can't buy obsolete vehicles!";
	}
	if (!cnv.is_bound()  &&  convoihandle_t::is_exhausted()) {
		return "invalid: Convoi handles exhausted!";
	}
	const uint32 count = cnv.is_bound() ? cnv->get_vehicle_count() : 0;
	if (count + list.get_count() > depot->get_max_convoi_length()) {
		return depot_error("invalid: the convoy would have %u vehicles, the depot allows %u", count + list.get_count(), depot->get_max_convoi_length());
	}
	// see depot_frame_t::image_from_storage_list: tool = 'a' or 'i'
	call_tool_init call = depot_tool(depot, player, infront ? 'i' : 'a', cnv, desc->get_name());
	call.require_funds(depot->get_owner(), -depot_purchase_cost(depot, list));
	return call;
}

call_tool_init depot_append_vehicle(depot_t *depot, player_t *player, convoihandle_t cnv, const vehicle_desc_t *desc)
{
	return depot_add_vehicle(depot, player, cnv, desc, false);
}

call_tool_init depot_insert_vehicle(depot_t *depot, player_t *player, convoihandle_t cnv, const vehicle_desc_t *desc)
{
	return depot_add_vehicle(depot, player, cnv, desc, true);
}

call_tool_init depot_sell_vehicle(depot_t *depot, player_t *player, const vehicle_desc_t *desc)
{
	if (desc == NULL) {
		return "invalid: Invalid vehicle_desc_x provided";
	}
	// the tool ignores it silently
	if (depot_stock(depot, desc) == 0) {
		return "invalid: no such vehicle is stored in this depot";
	}
	// see depot_frame_t::image_from_storage_list: tool = 's', sells the newest one (and the stored ones that belong to it)
	return depot_tool(depot, player, 's', convoihandle_t(), desc->get_name());
}

call_tool_init depot_sell_all_vehicles(depot_t *depot, player_t *player)
{
	// see depot_frame_t::action_triggered: tool = 'S'
	return depot_tool(depot, player, 'S', convoihandle_t(), NULL);
}

/// common part of remove_vehicle and remove_vehicles_to_end (tools 'r' and 'R')
static call_tool_init depot_remove(depot_t *depot, player_t *player, convoihandle_t cnv, sint32 index, bool to_end)
{
	if (!cnv.is_bound()  ||  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	if (index < 0  ||  index >= cnv->get_vehicle_count()) {
		return depot_error("invalid: the convoy has no vehicle %d (it has %u)", index, cnv->get_vehicle_count());
	}
	// vehicles that have to be together are removed together:
	// find the start as depot_frame_t::image_from_convoi_list does, the tool finds the end
	sint32 start_nr = index;
	while (start_nr > 0) {
		start_nr--;
		const vehicle_desc_t *info = cnv->get_vehikel(start_nr)->get_desc();
		if (info->get_trailer_count() != 1  ||  info->get_trailer(0) == vehicle_desc_t::any_vehicle  ||  cnv->is_invalid_convoy()) {
			start_nr++;
			break;
		}
	}
	char start[16];
	sprintf(start, "%d", start_nr);
	return depot_tool(depot, player, to_end ? 'R' : 'r', cnv, start);
}

call_tool_init depot_remove_vehicle(depot_t *depot, player_t *player, convoihandle_t cnv, sint32 index)
{
	return depot_remove(depot, player, cnv, index, false);
}

call_tool_init depot_remove_vehicles_to_end(depot_t *depot, player_t *player, convoihandle_t cnv, sint32 index)
{
	return depot_remove(depot, player, cnv, index, true);
}

/// common part of disassemble_convoy and sell_convoy (tools 'd' and 'v')
static call_tool_init depot_disassemble(depot_t *depot, player_t *player, convoihandle_t cnv, bool sell)
{
	if (!cnv.is_bound()  ||  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	// see depot_frame_t::action_triggered: tool = 'd' or 'v'
	return depot_tool(depot, player, sell ? 'v' : 'd', cnv, NULL);
}

call_tool_init depot_disassemble_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	return depot_disassemble(depot, player, cnv, false);
}

call_tool_init depot_sell_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	return depot_disassemble(depot, player, cnv, true);
}

call_tool_init depot_disassemble_all(depot_t *depot, player_t *player, bool sell)
{
	// see depot_frame_t::action_triggered: tool = 'D' or 'V' (ctrl-click)
	return depot_tool(depot, player, sell ? 'V' : 'D', convoihandle_t(), NULL);
}

call_tool_init depot_copy_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	if (!cnv.is_bound()  ||  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	// the tool also copies the convoy coupled behind (depot_t::copy_convoi ignores empty convoys and other waytypes)
	vector_tpl<const vehicle_desc_t*> list;
	uint32 copies = 0;
	for (convoihandle_t c = cnv;  c.is_bound()  &&  copies < 2;  c = c->get_coupling_convoi()) {
		if (c->get_vehicle_count() == 0  ||  depot->get_waytype() != c->front()->get_desc()->get_waytype()) {
			if (c == cnv) {
				return "invalid: the convoy has no vehicles of the waytype of this depot";
			}
			break;
		}
		for (uint32 i = 0;  i < c->get_vehicle_count();  i++) {
			list.append(c->get_vehikel_in_normal_order(i)->get_desc());
		}
		copies++;
	}
	// as depot_frame_t::action_triggered (bt_copy_convoi)
	if (welt->use_timeline()  &&  !welt->get_settings().get_allow_buying_obsolete_vehicles()  &&  !depot->check_obsolete_inventory(cnv)) {
		return "invalid: Can't buy obsolete vehicles!";
	}
	if (convoihandle_t::is_exhausted()) {
		return "invalid: Convoi handles exhausted!";
	}
	// see depot_frame_t::action_triggered: tool = 'c'
	call_tool_init call = depot_tool(depot, player, 'c', cnv, NULL);
	call.require_funds(depot->get_owner(), -depot_purchase_cost(depot, list));
	return call;
}

call_tool_init depot_reverse_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	if (!cnv.is_bound()  ||  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	// see depot_frame_t::action_triggered: tool = 't', toggles convoi_t::is_reversing_needed
	return depot_tool(depot, player, 't', cnv, NULL);
}

call_tool_init depot_couple_convoy(depot_t *depot, player_t *player, convoihandle_t cnv, convoihandle_t child)
{
	slist_tpl<convoihandle_t> const& convoys = depot->get_convoy_list();
	if (!cnv.is_bound()  ||  !convoys.is_contained(cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	if (child.is_bound()) {
		if (!convoys.is_contained(child)) {
			return "invalid: the convoy to couple is not in this depot";
		}
		// see depot_frame_t::action_triggered (child_convoi_selector)
		for (convoihandle_t c = child;  c.is_bound();  c = c->get_coupling_convoi()) {
			if (c == cnv) {
				return "invalid: This convoy cannot be coupled because it is a parent of shown convoy!";
			}
		}
	}
	// see depot_frame_t::action_triggered: tool = 'u', 0 uncouples
	char id[16];
	sprintf(id, "%u", child.is_bound() ? (uint32)child.get_id() : 0u);
	return depot_tool(depot, player, 'u', cnv, id);
}

call_tool_init depot_set_replacement_seed(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	if (!cnv.is_bound()  ||  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	// see depot_frame_t::action_triggered: tool = 'e', toggles
	return depot_tool(depot, player, 'e', cnv, NULL);
}

convoihandle_t depot_get_replacement_seed(depot_t *depot)
{
	return depot->get_replacement_seed();
}

call_tool_init depot_apply_template(depot_t *depot, player_t *player, convoihandle_t cnv, const char *name, const char *mode, vector_tpl<const vehicle_desc_t*> const& vehicles)
{
	if (cnv.is_bound()  &&  !convoy_in_depot(depot, cnv)) {
		return "invalid: the convoy is not in this depot";
	}
	if (mode == NULL  ||  (strcmp(mode, "a") != 0  &&  strcmp(mode, "i") != 0)) {
		return "invalid: mode must be \"a\" (append) or \"i\" (insert)";
	}
	if (name  &&  (strchr(name, '=')  ||  strlen(name) > 255)) {
		return "invalid: the name must not contain '=' and have at most 255 bytes";
	}
	if (vehicles.empty()) {
		return "invalid: no vehicles";
	}
	// the tool skips these silently
	for (uint32 i = 0;  i < vehicles.get_count();  i++) {
		const vehicle_desc_t *desc = vehicles[i];
		if (desc == NULL) {
			return depot_error("invalid: vehicle %u is not a vehicle_desc_x", i);
		}
		if (!depot->can_accept_waytype(desc->get_waytype())) {
			return depot_error("invalid: vehicle %u (%s) is for another waytype", i, desc->get_name());
		}
		if (!depot_can_buy(depot, desc)) {
			return depot_error("invalid: vehicle %u (%s): Can't buy obsolete vehicles!", i, desc->get_name());
		}
	}
	if (!cnv.is_bound()  &&  convoihandle_t::is_exhausted()) {
		return "invalid: Convoi handles exhausted!";
	}
	const uint32 count = cnv.is_bound() ? cnv->get_vehicle_count() : 0;
	if (count + vehicles.get_count() > depot->get_max_convoi_length()) {
		return depot_error("invalid: the convoy would have %u vehicles, the depot allows %u", count + vehicles.get_count(), depot->get_max_convoi_length());
	}
	// see depot_frame_t::action_triggered (template_panel): tool = 'T', [convoy name]=a|i=vehicle=vehicle...
	// the vehicles are given in the order of the convoy; inserting in front needs them reversed
	const bool infront = mode[0] == 'i';
	cbuffer_t buf;
	buf.append(name ? name : "");
	buf.append("=");
	buf.append(mode);
	for (uint32 i = 0;  i < vehicles.get_count();  i++) {
		buf.append("=");
		buf.append(vehicles[infront ? vehicles.get_count() - 1 - i : i]->get_name());
	}
	call_tool_init call = depot_tool(depot, player, 'T', cnv, buf);
	call.require_funds(depot->get_owner(), -depot_purchase_cost(depot, vehicles));
	return call;
}

/// apply_template(pl, cnv or null, name or null, mode, [vehicle_desc_x, ...])
SQInteger depot_apply_template_sq(HSQUIRRELVM vm)
{
	depot_t *depot = param<depot_t*>::get(vm, 1);
	player_t *player = param<player_t*>::get(vm, 2);
	convoihandle_t cnv = param<convoihandle_t>::get(vm, 3);
	const char *name = sq_gettype(vm, 4) == OT_NULL ? NULL : param<const char*>::get(vm, 4);
	const char *mode = param<const char*>::get(vm, 5);
	vector_tpl<const vehicle_desc_t*> vehicles;
	sq_pushnull(vm);
	while (SQ_SUCCEEDED(sq_next(vm, 6))) {
		vehicles.append(param<const vehicle_desc_t*>::get(vm, -1));
		sq_pop(vm, 2);
	}
	sq_pop(vm, 1);
	if (depot == NULL) {
		return sq_raise_error(vm, "invalid: no depot");
	}
	return param<call_tool_init>::push(vm, depot_apply_template(depot, player, cnv, name, mode, vehicles));
}

call_tool_init depot_set_name(depot_t *depot, player_t *player, const char *name)
{
	// the name input of depot_frame_t takes at most 59 bytes
	if (name == NULL  ||  *name == 0) {
		return "invalid: no name";
	}
	if (strlen(name) > 59) {
		return "invalid: the name is longer than 59 bytes";
	}
	// see depot_frame_t::action_triggered: tool = 'N'
	return depot_tool(depot, player, 'N', convoihandle_t(), name);
}

/**
 * Why the depot would not let the convoy depart (depot_t::start_convoi and can_start_convoi,
 * without their side effects), NULL if it would.
 * The route is searched as there, but the route of an aircraft is not checked.
 */
static const char *depot_start_failure(depot_t *depot, convoihandle_t cnv)
{
	if (depot->find_parent_convoy_in_depot(cnv).is_bound()) {
		return depot_error("invalid: the convoy %s is coupled behind another convoy and cannot depart alone", cnv->get_name());
	}
	const schedule_t *schedule = cnv->get_schedule();
	if (schedule == NULL  ||  schedule->empty()) {
		return "invalid: the convoy has no schedule";
	}
	// a single entry at another depot of the owner: the convoy moves there
	if (schedule->get_count() == 1) {
		if (grund_t *gr = welt->lookup(schedule->at(0).pos)) {
			depot_t *dep = gr->get_depot();
			if (dep  &&  dep->get_owner() == depot->get_owner()  &&  cnv->get_vehicle_count() > 0  &&  dep->can_accept_waytype(cnv->front()->get_desc()->get_waytype())) {
				return NULL;
			}
		}
	}
	const convoihandle_t front_cnv = depot->find_most_parent_convoy_in_depot(cnv);
	koord3d parent_stop = koord3d::invalid;
	for (convoihandle_t c = cnv;  c.is_bound();  c = c->get_coupling_convoi()) {
		const schedule_t *s = c->get_schedule();
		if (s == NULL  ||  s->empty()) {
			return depot_error("invalid: the coupled convoy %s has no schedule", c->get_name());
		}
		// the next stop of a coupled convoy is compared before its depot entry is skipped
		koord3d stop = s->get_current_entry().pos;
		if (c != cnv  &&  stop != parent_stop) {
			return depot_error("invalid: the convoy %s will couple with %s, but the next stops are different", cnv->get_name(), c->get_name());
		}
		if (stop == depot->get_pos()) {
			stop = s->at((s->get_current_stop() + 1) % s->get_count()).pos;
		}
		parent_stop = stop;
		if (front_cnv->get_total_sum_power() == 0) {
			return "invalid: the convoy has no power";
		}
		if (!c->pruefe_alle()  &&  !c->is_invalid_convoy()) {
			return depot_error("invalid: the vehicles of %s cannot be combined", c->get_name());
		}
		// the route as vehicle_t::calc_route searches it, into a route of our own
		vehicle_t *v = front_cnv->get_vehicle_count() > 0 ? front_cnv->get_vehikel(0) : NULL;
		if (v  &&  v->get_waytype() != air_wt) {
			// see convoi_t::calc_min_top_speed and convoi_t::check_electrification
			sint32 min_speed = SPEED_UNLIMITED;
			for (convoihandle_t p = front_cnv;  p.is_bound();  p = p->get_coupling_convoi()) {
				for (uint8 i = 0;  i < p->get_vehicle_count();  i++) {
					min_speed = min(min_speed, kmh_to_speed(p->get_vehikel(i)->get_desc()->get_topspeed()));
				}
			}
			const convoihandle_t most_parent = front_cnv->get_most_parent_convoi();
			bool need_electric = false;
			for (convoihandle_t p = most_parent;  p.is_bound();  p = p->get_coupling_convoi()) {
				if (!p->is_invalid_convoy()  &&  (!p->get_schedule()  ||  !p->get_schedule()->is_no_use_electric())) {
					for (uint8 i = 0;  i < p->get_vehicle_count();  i++) {
						need_electric |= p->get_vehikel(i)->get_desc()->get_engine_type() == vehicle_desc_t::electric;
					}
				}
			}
			for (convoihandle_t p = most_parent;  p.is_bound()  &&  need_electric;  p = p->get_coupling_convoi()) {
				if (!p->is_invalid_convoy()  &&  (!p->get_schedule()  ||  !p->get_schedule()->is_no_use_electric())) {
					for (uint8 i = 0;  i < p->get_vehicle_count();  i++) {
						const vehicle_desc_t *d = p->get_vehikel(i)->get_desc();
						need_electric &= !(d->get_engine_type() != vehicle_desc_t::electric  &&  d->get_power() > 0);
					}
				}
			}
			// road and rail vehicles search with electrification first and then with need_electric: the second decides
			route_t route;
			if (!route.calc_route(welt, depot->get_pos(), stop, v, speed_to_kmh(min_speed), 0, v->get_waytype() != water_wt  &&  need_electric)) {
				return depot_error("invalid: the convoy %s can't find a route to (%s)", cnv->get_name(), stop.get_str());
			}
		}
	}
	return NULL;
}

call_tool_init depot_start_convoy(depot_t *depot, player_t *player, convoihandle_t cnv)
{
	// see depot_frame_t::action_triggered: tool = 'b'
	if (cnv.is_bound()) {
		// the tool ignores these silently (depot_t::start_convoi)
		if (!convoy_in_depot(depot, cnv)) {
			return "invalid: the convoy is not in this depot";
		}
		if (const char *err = depot_start_failure(depot, cnv)) {
			return err;
		}
		return depot_tool(depot, player, 'b', cnv, NULL);
	}
	return depot_tool(depot, player, 'B', cnv, NULL);
}

/// the vehicles stored in the depot (in no convoy): [{desc, value, purchase_time}, ...]
SQInteger depot_get_stored_vehicles(HSQUIRRELVM vm)
{
	depot_t *depot = param<depot_t*>::get(vm, 1);
	sq_newarray(vm, 0);
	if (depot == NULL) {
		return 1;
	}
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		sq_newtable(vm);
		const SQInteger table = sq_gettop(vm);
		sq_pushstring(vm, "desc", -1);
		param<const vehicle_desc_t*>::push(vm, v->get_desc());
		sq_newslot(vm, table, false);
		create_slot(vm, "value", (sint64)v->calc_sale_value(), false, table);
		create_slot(vm, "purchase_time", (sint32)v->get_purchase_time(), false, table);
		sq_arrayappend(vm, -2);
	}
	return 1;
}

/// the vehicles that can be bought in this depot now (as depot_frame_t::build_vehicle_lists lists them)
vector_tpl<const vehicle_desc_t*> const& depot_get_buyable_vehicles(depot_t *depot)
{
	static vector_tpl<const vehicle_desc_t*> list;
	list.clear();
	if (depot == NULL) {
		return list;
	}
	const uint16 month_now = welt->get_timeline_year_month();
	const bool obsolete = welt->get_settings().get_allow_buying_obsolete_vehicles();
	const waytype_t wt = depot->get_waytype();
	const grund_t *gr = welt->lookup(depot->get_pos());
	const weg_t *w = gr ? gr->get_weg(wt != tram_wt ? wt : track_wt) : NULL;
	const bool electrified = w  &&  w->is_electrified();
	const waytype_t sec_wt = depot->get_secondary_waytype();
	for (int pass = 0;  pass < 2;  pass++) {
		if (pass == 1  &&  sec_wt == invalid_wt) {
			break;
		}
		slist_tpl<const vehicle_desc_t*> const& types = pass == 0 ? depot->get_vehicle_type() : vehicle_builder_t::get_info(sec_wt);
		FOR(slist_tpl<const vehicle_desc_t*>, const info, types) {
			const bool stored = depot_stock(depot, info) > 0;
			if (stored  ||  ((electrified  ||  info->get_engine_type() != vehicle_desc_t::electric)  &&  !info->is_future(month_now)  &&  (obsolete  ||  !info->is_retired(month_now)))) {
				if (!list.is_contained(info)) {
					list.append(info);
				}
			}
		}
	}
	return list;
}

vector_tpl<convoihandle_t> const& depot_get_convoy_list(depot_t *depot)
{
	static vector_tpl<convoihandle_t> list;
	list.clear();
	if (depot==NULL) {
		return list;
	}
	// fill list
	slist_tpl<convoihandle_t> const& slist = depot->get_convoy_list();

	for(slist_tpl<convoihandle_t>::const_iterator i = slist.begin(), end = slist.end(); i!=end; ++i) {
		list.append(*i);
	}
	return list;
}

vector_tpl<depot_t*> const& get_depot_list(player_t *player, waytype_t wt)
{
	static vector_tpl<depot_t*> list;
	list.clear();
	// do the conversion of waytype_t to depot-type by linetype
	simline_t::linetype line_type = simline_t::waytype_to_linetype(wt);
	if (player == NULL  ||  line_type == simline_t::MAX_LINE_TYPE) {
		return list;
	}
	// fill list
	const slist_tpl<depot_t*> &depot_list = depot_t::get_depot_list();

	FOR(slist_tpl<depot_t*>, d, depot_list) {
		if(d->get_line_type()==line_type  &&  d->get_owner()==player) {
			list.append(d);
		}
	}
	return list;
}


const fabrik_t* transformer_get_factory(leitung_t *lt)
{
	if (pumpe_t *p = dynamic_cast<pumpe_t*>(lt)) {
		return p->get_factory();
	}
	if (senke_t *s = dynamic_cast<senke_t*>(lt)) {
		return s->get_factory();
	}
	return NULL;
}

bool leitung_is_connected(leitung_t* lt1, leitung_t* lt2)
{
	return lt2 != NULL  &&  lt1->get_net() == lt2->get_net();
}

bool field_is_deleteable(field_t* f)
{
	return f->is_deletable( welt->get_player(1) ) == NULL;
}


vector_tpl<sint64> const& get_way_stat(weg_t* weg, sint32 INDEX)
{
	static vector_tpl<sint64> v;
	v.clear();
	if (weg  &&  0<=INDEX  &&  INDEX<WAY_STAT_MAX) {
		for(uint16 i = 0; i < MAX_WAY_STAT_MONTHS; i++) {
			v.append( weg->get_stat(i, INDEX) );
		}
	}
	return v;
}

vector_tpl<grund_t*> const& get_tile_list( gebaeude_t *gb )
{
	static vector_tpl<grund_t*> list;
	gb->get_tile_list( list );
	return list;
}

void export_map_objects(HSQUIRRELVM vm)
{
	/**
	 * Class to access objects on the map
	 * These classes cannot modify anything.
	 */
	begin_class(vm, "map_object_x", "extend_get,coord3d,ingame_object");
	uint8 objtype = bind_code<obj_t>::objtype;
	sq_settypetag(vm, -1, obj_t_tag + objtype);
	/**
	 * Constructor. Implemented by derived classes.
	 * Fails if no object of precisely the requested type is on the tile.
	 * If there is more than one object of this type on the tile then it will return the first.
	 * @param x
	 * @param y
	 * @param z
	 * @param type of the map object
	 * @typemask void(integer,integer,integer,map_objects)
	 */
	register_function(vm, exp_obj_pos_constructor, "constructor", 5, "xiiii");
	/**
	 * @returns if object is still valid.
	 */
	export_is_valid<obj_t*>(vm); //register_function("is_valid")
	/**
	 * @returns owner of the object.
	 */
	register_method(vm, &obj_t::get_owner, "get_owner");
	/**
	 * @returns raw name.
	 */
	register_method(vm, &obj_t::get_name, "get_name");
	/**
	 * @returns way type, can be @ref wt_invalid.
	 */
	register_method(vm, &obj_t::get_waytype, "get_waytype");
	/**
	 * @returns position.
	 */
	register_method(vm, &obj_t::get_pos, "get_pos");
	/**
	 * Checks whether player can remove this object.
	 * @returns error message or null if object can be removed.
	 */
	register_method(vm, &obj_t::is_deletable, "is_removable");
	/**
	 * @returns type of object.
	 */
	register_method(vm, &obj_t::get_typ, "get_type");
	/**
	 * Marks the object for highlighting. Use with care.
	 */
	register_method(vm, &mark_object, "mark", true);
	/**
	 * Unmarks the object for highlighting.
	 */
	register_method(vm, &unmark_object, "unmark", true);
	/**
	 * @returns whether object is highlighted.
	 */
	register_method(vm, &object_is_marked, "is_marked", true);
	end_class(vm);


	/**
	 * Trees on the map.
	 */
	begin_obj_class<baum_t>(vm, "tree_x", "map_object_x");
	/**
	 * @returns age of tree in months.
	 */
	register_method(vm, &baum_t::get_age, "get_age");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &baum_t::get_desc, "get_desc");

	end_class(vm);

	/**
	 * Buildings.
	 */
	begin_obj_class<gebaeude_t>(vm, "building_x", "map_object_x");
	/**
	 * @returns factory if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_fabrik, "get_factory");
	/**
	 * @returns city if building belongs to one, otherwise null
	 */
	register_method(vm, &gebaeude_t::get_stadt, "get_city");
	/**
	 * @returns whether building is townhall
	 */
	register_method(vm, &gebaeude_t::is_townhall, "is_townhall");
	/**
	 * @returns whether building is headquarters
	 */
	register_method(vm, &gebaeude_t::is_headquarter, "is_headquarter");
	/**
	 * @returns whether building is a monument
	 */
	register_method(vm, &gebaeude_t::is_monument, "is_monument");
	/**
	 * Passenger level controls how many passengers will be generated by this building.
	 * @returns passenger level
	 */
	register_method(vm, &gebaeude_t::get_passagier_level, "get_passenger_level");
	/**
	 * Mail level controls how many mail will be generated by this building.
	 * @returns mail level
	 */
	register_method(vm, &gebaeude_t::get_mail_level, "get_mail_level");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &gebaeude_t::get_tile, "get_desc");
	/**
	 * @returns coord for all tiles
	 */
	register_method( vm, &get_tile_list, "get_tile_list", true );
	/**
	 * @returns true if both building tiles are part of one (multi-tile) building
	 */
	register_method(vm, &gebaeude_t::is_same_building, "is_same_building");

	end_class(vm);

	/**
	 * Class to access depots.
	 * Main purpose is the manipulation of convoys in depots.
	 */
	begin_obj_class<depot_t>(vm, "depot_x", "building_x");
	/**
	 * Append a car to the convoy in this depot. If the convoy does not exist, a new one is created first.
	 * As in the depot window, the vehicles that have to follow it are appended with it.
	 * Uses the vehicles stored in the depot first, and buys the others.
	 * @param pl player owns the convoy
	 * @param cnv the convoy
	 * @param desc decriptor of the vehicle
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_append_vehicle, "append_vehicle", true);
	/**
	 * Insert a car in front of the convoy in this depot. If the convoy does not exist, a new one is created first.
	 * As in the depot window, the vehicles that have to lead it are inserted with it.
	 * @param pl player owns the convoy
	 * @param cnv the convoy
	 * @param desc decriptor of the vehicle
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_insert_vehicle, "insert_vehicle", true);
	/**
	 * Sell a vehicle stored in this depot (the newest one of this type), with the stored vehicles that belong to it.
	 * @param pl player owns the depot
	 * @param desc decriptor of the vehicle
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_sell_vehicle, "sell_vehicle", true);
	/**
	 * Sell all vehicles stored in this depot.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_sell_all_vehicles, "sell_all_vehicles", true);
	/**
	 * Remove a vehicle from a convoy in this depot and store it in the depot.
	 * As in the depot window, the vehicles that belong together are removed together.
	 * If no vehicle remains, the convoy is disassembled.
	 * @param pl player owns the depot
	 * @param cnv the convoy
	 * @param index position of the vehicle in the convoy (0 is the front)
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_remove_vehicle, "remove_vehicle", true);
	/**
	 * Remove a vehicle and all vehicles behind it from a convoy in this depot and store them in the depot.
	 * @param pl player owns the depot
	 * @param cnv the convoy
	 * @param index position of the first vehicle to remove (0 is the front)
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_remove_vehicles_to_end, "remove_vehicles_to_end", true);
	/**
	 * Disassemble a convoy in this depot: its vehicles are stored in the depot.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_disassemble_convoy, "disassemble_convoy", true);
	/**
	 * Sell a convoy in this depot.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_sell_convoy, "sell_convoy", true);
	/**
	 * Disassemble all convoys in this depot.
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, depot_disassemble_all, "disassemble_all", freevariable<bool>(false), true);
	/**
	 * Sell all convoys in this depot.
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, depot_disassemble_all, "sell_all_convoys", freevariable<bool>(true), true);
	/**
	 * Copy a convoy in this depot (and the convoy coupled behind it), with its schedule or line.
	 * Uses the vehicles stored in the depot first, and buys the others.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_copy_convoy, "copy_convoy", true);
	/**
	 * Toggle the direction in which the convoy in this depot will depart (the reverse button of the depot window).
	 * @see convoy_x::is_reversing_needed
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_reverse_convoy, "reverse_convoy", true);
	/**
	 * Couple a convoy of this depot behind a convoy of this depot.
	 * @param pl player owns the depot
	 * @param cnv the convoy in front
	 * @param child the convoy behind, convoy_x(0) uncouples
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_couple_convoy, "couple_convoy", true);
	/**
	 * Make the convoy the replacement seed of this depot, or no longer if it is already.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_set_replacement_seed, "set_replacement_seed", true);
	/**
	 * @returns the replacement seed of this depot (or null)
	 */
	register_method(vm, depot_get_replacement_seed, "get_replacement_seed", true);
	/**
	 * Add the vehicles of a convoy template to a convoy in this depot (a new convoy if null).
	 * @param pl player owns the depot
	 * @param cnv the convoy, or null for a new one
	 * @param name name of a new convoy (empty: the game chooses)
	 * @param mode "a" appends the vehicles at the end, "i" inserts them in front
	 * @param vehicles array of vehicle_desc_x, in the order of the convoy (also for "i")
	 * @ingroup game_cmd
	 */
	register_function(vm, depot_apply_template_sq, "apply_template", 6, "x....a");
	/**
	 * Rename this depot.
	 * @param pl player owns the depot
	 * @param name the new name (1 to 59 bytes)
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_set_name, "set_name", true);
	/**
	 * Start the convoy in this depot.
	 * Fails with the reason as the depot window shows it (coupled behind another convoy,
	 * no schedule, no power, invalid combination, no route, different next stops of coupled convoys).
	 * The route of aircraft is not checked.
	 * @ingroup game_cmd
	 */
	register_method(vm, depot_start_convoy, "start_convoy", true);
	/**
	 * Start all convoys in this depot.
	 * @ingroup game_cmd
	 */
	register_method_fv(vm, depot_start_convoy, "start_all_convoys", freevariable<convoihandle_t>(convoihandle_t()), true);
	/**
	 * @returns list of convoys sitting in this depot
	 */
	register_method(vm, &depot_get_convoy_list, "get_convoy_list", true);
	/**
	 * @returns the vehicles stored in this depot (in no convoy): array of tables
	 *   {desc (vehicle_desc_x), value (what selling it gives, 1/100 credits), purchase_time (months)}
	 * @typemask array()
	 */
	register_function(vm, depot_get_stored_vehicles, "get_stored_vehicles", 1, "x");
	/**
	 * @returns the vehicles that can be bought in this depot now, as the depot window lists them
	 *   (of its waytypes, electric ones only on electrified ways, not future ones, obsolete ones
	 *   only if the settings allow buying them or if they are stored in the depot)
	 */
	register_method(vm, &depot_get_buyable_vehicles, "get_buyable_vehicles", true);
	/**
	 * List of all depots of player @p pl of way-type @p wt
	 * @param pl
	 * @param wt
	 * @returns depot list of player
	 */
	STATIC register_method(vm, &get_depot_list, "get_depot_list", false, true);
	end_class(vm);

	/**
	 * Ways.
	 */
	begin_obj_class<weg_t>(vm, "way_x", "map_object_x");
	/**
	 * @return if this way has sidewalk - only meaningful for roads
	 */
	register_method(vm, &weg_t::hat_gehweg, "has_sidewalk");
	/**
	 * @return whether way is electrified by some way-object
	 */
	register_method(vm, &weg_t::is_electrified, "is_electrified");
	/**
	 * @return whether there is a road-sign associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_sign, "has_sign");
	/**
	 * @return whether there is a signal associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_signal, "has_signal");
	/**
	 * @return whether there is a way-object associated to the way on the tile
	 */
	register_method(vm, &weg_t::has_wayobj, "has_wayobj");
	/**
	 * @return whether there is a crossing associated to the way on the tile
	 */
	register_method(vm, &weg_t::is_crossing, "is_crossing");
	/**
	 * Return directions of this way. One-way signs are ignored here.
	 * @returns direction
	 * @typemask dir()
	 */
	register_function_fv(vm, &get_way_ribi, "get_dirs", 1, "x", freevariable<bool>(false) );
	/**
	 * Return directions of this way. Some signs restrict available directions.
	 * @returns direction
	 * @typemask dir()
	 */
	register_function_fv(vm, &get_way_ribi, "get_dirs_masked", 1, "x", freevariable<bool>(true) );
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &weg_t::get_desc, "get_desc");
	/**
	 * Returns maximal allowed speed on this way.
	 * Takes limits from crossings, overhead-wires, bridges, etc, into account.
	 * @returns max speed in kmh.
	 */
	register_method(vm, &weg_t::get_max_speed, "get_max_speed");
	/**
	 * Get monthly statistics of goods transported on this way.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_way_stat, "get_transported_goods", freevariable<sint32>(WAY_STAT_GOODS), true);
	/**
	 * Get monthly statistics of convoys that used this way.
	 * @returns array, index [0] corresponds to current month
	 */
	register_method_fv(vm, &get_way_stat, "get_convoys_passed",    freevariable<sint32>(WAY_STAT_CONVOIS), true);
	end_class(vm);

	/**
	 * Streets (roads)
	 */
	begin_obj_class<strasse_t>(vm, "street_x", "way_x");
	/**
	 * @return overtaking mode
	 */
	register_function(vm, &get_overtaking_mode, "get_overtaking_mode", 1, "x");
	/**
	 * @return street flags
	 */
	register_function(vm, &get_street_flags, "get_street_flags", 1, "x");
	end_class(vm);

	/**
	 * Labels.
	 */
	begin_obj_class<label_t>(vm, "label_x", "map_object_x");
	/**
	 * Creates a new marker.
	 * @param pos  position
	 * @param pl   owner
	 * @param text text
	 * @ingroup game_cmd
	 */
	STATIC register_method(vm, &create_marker, "create", false, true);
	/**
	 * Set text of marker.
	 * @param text text
	 * @ingroup rename_func
	 */
	register_method(vm, &label_set_text, "set_text", true);
	/**
	 * Get text of marker.
	 * @see tile_x::get_text
	 */
	register_method(vm, &label_get_text, "get_text", true);

	end_class(vm);

	/**
	 * Roadsigns and railway signals.
	 */
	begin_obj_class<roadsign_t>(vm, "sign_x", "map_object_x");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &roadsign_t::get_desc, "get_desc");

	/**
	 * Test whether @p player 's vehicles can pass private-way sign.
	 * Returns true if this is not a private-way sign.
	 * @param player
	 */
	register_method(vm, &roadsign_can_pass, "can_pass", true);

	/**
	 * Get signal state.
	 * @returns signal state: 0 = red, 1 = green, 2 = yellow
	 */
	register_method(vm, &roadsign_get_state, "get_state", true);

	/**
	 * Get "stop before check" flag of a signal.
	 * Applicable to simple signals, longblock signals and choose signals.
	 * When true, the signal forces the convoy to stop completely before the
	 * block-clearance check is performed (signal_clear is held false and
	 * restart_speed is set to -1 until the convoy is waiting).
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_stop_before_check, "is_stop_before_check");
	/**
	 * Get "start signal" flag of a signal.
	 * When true, a convoy in CAN_START or CAN_START_ONE_MONTH state will check
	 * whether the next signal ahead is clear before departing.  If that signal
	 * is RED the convoy stays at its current position (does not advance to the
	 * signal tile).
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_start_signal, "is_start_signal");
	/**
	 * Get "advance to end" flag of a choose signal.
	 * When true, trains routed to this choose signal will try to advance to the end of the platform.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_advance_to_end, "is_advance_to_end");
	/**
	 * Get "require parent convoy to enter" (guide signal) flag of a signal.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_guide_signal, "is_guide_signal");
	/**
	 * Get "choose signal" flag of a signal.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_choose_signal, "is_choose_signal");
	/**
	 * Get "skip default route" flag of a choose signal.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_skip_default_route, "is_skip_default_route");
	/**
	 * Get "length based" flag of a choose signal.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::is_length_based, "is_length_based");
	/**
	 * Get margin length of a choose signal.
	 * @returns margin length in tiles
	 */
	register_method(vm, &roadsign_t::get_margin_length, "get_margin_length");
	/**
	 * Get "allow reverse passage" (two ways) flag of a signal.
	 * When true, trains coming from the reverse direction are allowed to pass the signal.
	 * @returns true if the flag is set
	 */
	register_method(vm, &roadsign_t::get_two_ways, "get_two_ways");
	end_class(vm);

	/**
	 * Powerlines.
	 * (Including transformers. Can be differentiated by @ref obj_x::get_type.
	 */
	begin_obj_class<leitung_t>(vm, "powerline_x", "map_object_x");

	/**
	 * Checkes whether powerline (or transformer) is connected to @p pl are connected.
	 * @param pl other powerline/transformer
	 */
	register_method(vm, &leitung_is_connected, "is_connected", true);
	end_class(vm);

	/**
	 * Transformers.
	 * Sink and source can be differentiated by @ref obj_x::get_type.
	 */
	begin_obj_class<pumpe_t>(vm, "transformer_x", "powerline_x");

	/**
	 * Get factory connected to this transformer.
	 */
	register_method(vm, &transformer_get_factory, "get_factory", true);
	end_class(vm);

	/**
	 * Fields
	 */
	begin_obj_class<field_t>(vm, "field_x", "map_object_x");
	/**
	 * Fields can only be deleted if enough are present.
	 * @see factory_x::get_field_count, factory_x::get_min_field_count
	 * @return whether this one can be deleted
	 */
	register_method(vm, &field_is_deleteable, "is_deletable", true);
	/**
	 * @returns factory this field belongs to
	 * @see factory_x::get_field_count, factory_x::get_min_field_count
	 */
	register_method(vm, &field_t::get_factory, "get_factory");
	end_class(vm);

	/**
	 * Way-objects: overhead-wires, fences, etc. Only one such object can exist on a tile.
	 */
	begin_obj_class<wayobj_t>(vm, "wayobj_x", "map_object_x");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &wayobj_t::get_desc, "get_desc");
	end_class(vm);

	/**
	 * Bridges: bridge objects of bridge ramps and tiles
	 */
	begin_obj_class<bruecke_t>(vm, "bridge_x", "map_object_x");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &bruecke_t::get_desc, "get_desc");
	end_class(vm);

	/**
	 * Tunnels
	 */
	begin_obj_class<tunnel_t>(vm, "tunnel_x", "map_object_x");
	/**
	 * @returns object descriptor.
	 */
	register_method(vm, &tunnel_t::get_desc, "get_desc");
	end_class(vm);
}
