/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/**
 * MCP tool implementations.
 *
 * run_squirrel accepts arbitrary Squirrel script code and executes it inside a
 * dedicated script VM that has the full Simutrans scripting API (ai_base)
 * available. capture_screen returns the current display as MCP image content.
 *
 * A fresh VM is created for each run_squirrel call and destroyed afterwards,
 * so no state (global variables, defined functions, etc.) leaks between calls.
 *
 * get_live_state, get_tiles, get_changes, dump_vehicles, get_info,
 * get_history and get_messages are the read-only tools for a live web view of
 * the game (see live_state.cc and live_info.cc).
 */

#include "mcp_tools.h"
#include "mcp_server.h"
#include "live_state.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <map>
#include <time.h>
#include <sys/stat.h>

#include "../script/script.h"
#include "../script/script_loader.h"
#include "../dataobj/environment.h"
#include "../display/simgraph.h"
#include "../utils/cbuffer_t.h"
#include "../utils/plainstring.h"
#include "../simconst.h"
#include "../simworld.h"
#include "../gui/minimap.h"
#include "../display/viewport.h"
#include "../display/simview.h"
#include "../sys/simsys.h"
#include "../dataobj/koord3d.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"
#include "../simversion.h"


// ---------------------------------------------------------------------------
// Minimal JSON helpers (output only)
// ---------------------------------------------------------------------------

static std::string jesc(const std::string &s) { return mcp_server_t::json_escape(s); }
static std::string jstr(const std::string &s)  { return "\"" + jesc(s) + "\""; }
static std::string jstr(const char *s)          { return s ? jstr(std::string(s)) : "null"; }

static std::string text_content(const std::string &text)
{
	return "{\"content\":[{\"type\":\"text\",\"text\":" + jstr(text) + "}]}";
}

static std::string base64_encode(const std::string &data)
{
	static const char chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string encoded;
	encoded.reserve(((data.size() + 2) / 3) * 4);

	for (size_t i = 0; i < data.size(); i += 3) {
		const uint32 a = (uint8)data[i];
		const uint32 b = i + 1 < data.size() ? (uint8)data[i + 1] : 0;
		const uint32 c = i + 2 < data.size() ? (uint8)data[i + 2] : 0;
		const uint32 value = (a << 16) | (b << 8) | c;

		encoded += chars[(value >> 18) & 0x3F];
		encoded += chars[(value >> 12) & 0x3F];
		encoded += i + 1 < data.size() ? chars[(value >> 6) & 0x3F] : '=';
		encoded += i + 2 < data.size() ? chars[value & 0x3F] : '=';
	}
	return encoded;
}


// ---------------------------------------------------------------------------
// Tool implementation
// ---------------------------------------------------------------------------

std::string mcp_tools::script_error(const script_vm_t *vm, const char *err)
{
	// a runtime error only says "Call function failed": the message the script raised is in the
	// error report of the error handler (_sqstd_aux_printerror: "<st>Error: [message]</st>")
	if (vm  &&  err  &&  strcmp(err, "Call function failed") == 0) {
		const std::string report = vm->get_error();
		const size_t start = report.find("Error: [");
		const size_t end = report.find("]</st>", start);
		if (start != std::string::npos  &&  end != std::string::npos) {
			return report.substr(start + 8, end - start - 8);
		}
	}
	return err ? err : "";
}

/// a script may run this many opcodes in all (10000 per frame) and this long (also waiting for its commands)
static const sint64 SCRIPT_MAX_OPS = 2000000;
static const uint32 SCRIPT_MAX_MS = 60000;

std::string mcp_tools::script_ops(const script_vm_t *vm)
{
	return ",\"ops\":" + live_changes_t::take_tool_results([vm](const void *v) { return vm  &&  vm->uses_vm((HSQUIRRELVM)v); });
}

uint32 mcp_tools::script_started_ms()
{
	return dr_time();
}

const char *mcp_tools::script_limit(const script_vm_t *vm, uint32 started_ms)
{
	if (vm->get_ops_total() > SCRIPT_MAX_OPS) {
		return "limit: the script ran more than 2000000 opcodes";
	}
	if (dr_time() - started_ms > SCRIPT_MAX_MS) {
		return "limit: the script ran longer than 60 seconds";
	}
	return NULL;
}

std::string mcp_tools::busy_result(const char *reason)
{
	std::string text = "{\"error\":" + jstr(std::string("busy: ") + reason) + ",\"busy\":" + jstr(reason)
		+ ",\"saving\":" + (strcmp(reason, "saving") == 0 ? "true" : "false")
		+ ",\"joining\":" + (live_changes_t::is_joining() ? "true" : "false")
		+ ",\"epoch\":" + std::to_string(live_changes_t::get_epoch()) + "}";
	return text_content(text);
}

// Start a squirrel script execution.
// Returns result JSON on immediate completion.
// On suspension (network mode command_x), sets *out_vm to the live VM and returns "".
// On error, returns error JSON.
// Caller owns *out_vm and must eventually delete it.
static std::string tool_run_squirrel(const std::string &code, int player_nr,
                                     script_vm_t **out_vm)
{
	// Create a fresh VM for each call so no state leaks between invocations.
	cbuffer_t ai_path;
	ai_path.printf("%sai/", env_t::data_dir);

	script_vm_t *vm = script_loader_t::start_vm("ai_base.nut", "script-mcp.log",
	                                              ai_path, /*is_scenario=*/false,
	                                              /*enable_io=*/false);
	if (!vm) {
		return "{\"error\":\"failed to initialize Squirrel VM\"}";
	}
	vm->set_my_player((uint8)player_nr);
	vm->set_mcp();
	vm->pause_on_error = false;

	// Wrap the user code in a named function so we can call it with
	// call_function<plainstring>(QUEUE, ...) and capture the return value.
	std::string wrapped =
		"function __mcp_fn__() {\n"
		+ code +
		"\n}";

	// Step 1: define __mcp_fn__ in the VM
	const char *err = vm->eval_string(wrapped.c_str());
	if (err && strcmp(err, "suspended") != 0) {
		std::string result_json = "{\"error\":" + jstr(err) + "}";
		delete vm;
		return result_json;
	}

	// Step 2: call __mcp_fn__ via QUEUE mode (allows command_x suspend in network mode)
	plainstring result;
	err = vm->call_function(script_vm_t::QUEUE, "__mcp_fn__", result);

	if (script_vm_t::is_call_suspended(err)) {
		// Script suspended waiting for a network command or for more opcodes — hand VM to caller
		*out_vm = vm;
		return "";
	}

	// done at once: the commands took effect in the current sync step
	const std::string sync = world() ? std::to_string(world()->get_sync_steps()) : "0";
	const std::string syncs = mcp_tools::script_ops(vm) + ",\"sync_start\":" + sync + ",\"sync\":" + sync + "}";
	std::string result_json;
	if (err) {
		result_json = "{\"error\":" + jstr(mcp_tools::script_error(vm, err)) + syncs;
	}
	else {
		result_json = "{\"result\":" + jstr(result.c_str()) + syncs;
	}
	delete vm;
	return result_json;
}

// Renders the world at a given position and zoom (without the windows) and returns it as PNG.
// The view of the player is restored afterwards. The image cannot be larger than the game window.
static std::string tool_capture_view(karte_t *welt, const std::string &args_json)
{
	if (!welt  ||  !welt->get_view()) {
		return text_content("{\"error\":\"world not ready\"}");
	}
	const std::string xs = mcp_server_t::json_get_raw(args_json, "x");
	const std::string ys = mcp_server_t::json_get_raw(args_json, "y");
	if (xs.empty()  ||  ys.empty()) {
		return text_content("{\"error\":\"x and y are required\"}");
	}
	const koord pos(atoi(xs.c_str()), atoi(ys.c_str()));
	if (!welt->is_within_limits(pos)) {
		return text_content("{\"error\":\"position outside the map\"}");
	}
	const std::string zs = mcp_server_t::json_get_raw(args_json, "z");
	const grund_t *gr = welt->lookup_kartenboden(pos);
	const sint8 z = zs.empty()  ||  zs == "null" ? (gr ? gr->get_hoehe() : 0) : (sint8)atoi(zs.c_str());
	const std::string zooms = mcp_server_t::json_get_raw(args_json, "zoom");
	const int zoom = zooms.empty()  ||  zooms == "null" ? get_zoom_factor() : atoi(zooms.c_str());
	if (zoom < 0  ||  zoom > 9) {
		return text_content("{\"error\":\"zoom: 0 (largest) to 9, 3 is 1:1\"}");
	}
	const std::string ws = mcp_server_t::json_get_raw(args_json, "w");
	const std::string hs = mcp_server_t::json_get_raw(args_json, "h");
	const scr_coord_val disp_w = display_get_width();
	const scr_coord_val disp_h = display_get_height();
	const scr_coord_val w = ws.empty() ? disp_w : min((scr_coord_val)atoi(ws.c_str()), disp_w);
	const scr_coord_val h = hs.empty() ? disp_h : min((scr_coord_val)atoi(hs.c_str()), disp_h);
	if (w <= 0  ||  h <= 0) {
		return text_content("{\"error\":\"bad size\"}");
	}

	viewport_t *view = welt->get_viewport();
	const koord old_pos = view->get_world_position();
	const sint16 old_x_off = view->get_x_off();
	const sint16 old_y_off = view->get_y_off();
	const int old_zoom = get_zoom_factor();

	set_zoom_factor_safe(zoom);
	view->metrics_updated();
	view->change_world_position(koord3d(pos, z));
	welt->get_view()->display(true);
	// the tile in the middle of the window
	const scr_rect area((disp_w - w) / 2, (disp_h - h) / 2, w, h);
	std::string png_data;
	const bool ok = display_snapshot_png(area, png_data);
	const scr_coord_val tile_width = get_tile_raster_width();

	set_zoom_factor_safe(old_zoom);
	view->metrics_updated();
	view->change_world_position(old_pos, old_x_off, old_y_off);
	welt->set_dirty();
	if (!ok) {
		return text_content("{\"error\":\"failed to capture the view\"}");
	}
	return "{\"content\":[{\"type\":\"image\",\"data\":"
		+ jstr(base64_encode(png_data))
		+ ",\"mimeType\":\"image/png\"},{\"type\":\"text\",\"text\":"
		+ jstr("{\"x\":" + std::to_string(pos.x) + ",\"y\":" + std::to_string(pos.y) + ",\"z\":" + std::to_string(z)
		       + ",\"zoom\":" + std::to_string(zoom) + ",\"tile_width\":" + std::to_string(tile_width)
		       + ",\"w\":" + std::to_string(w) + ",\"h\":" + std::to_string(h) + ",\"sync\":" + std::to_string(welt->get_sync_steps()) + "}")
		+ "}]}";
}

/// the answer of a save_world, kept for the calls within max_age
struct saved_world_t
{
	uint32 ms;          ///< dr_time() of the save
	uint32 epoch;
	std::string json;   ///< the fields after the head of the answer
};
static std::map<std::string, saved_world_t> saved_worlds;

/// a file name of letters, digits and _-. (not starting with a dot), at most 64 bytes, ending with .sve
static bool valid_save_name(const std::string &name)
{
	if(  name.size() < 5  ||  name.size() > 64  ||  name[0] == '.'  ||  name.compare(name.size() - 4, 4, ".sve") != 0  ) {
		return false;
	}
	for(  char c : name  ) {
		if(  !((c >= 'a'  &&  c <= 'z')  ||  (c >= 'A'  &&  c <= 'Z')  ||  (c >= '0'  &&  c <= '9')  ||  c == '_'  ||  c == '-'  ||  c == '.')  ) {
			return false;
		}
	}
	return name.find("..") == std::string::npos;
}

std::string mcp_tools::save_world(karte_t *welt, const std::string &args_json)
{
	if(  !welt  ) {
		return text_content("{\"error\":\"world not ready\"}");
	}
	const std::string name = mcp_server_t::json_get_string(args_json, "name");
	if(  !valid_save_name(name)  ) {
		return text_content("{\"error\":\"invalid: name must be a file name of letters, digits and _-. ending with .sve (at most 64 bytes)\"}");
	}
	const std::string max_age_raw = mcp_server_t::json_get_raw(args_json, "max_age");
	const sint64 max_age = max_age_raw.empty()  ||  max_age_raw == "null" ? 30 : atoll(max_age_raw.c_str());
	const std::string format = mcp_server_t::json_get_string(args_json, "format");
	loadsave_t::mode_t mode = loadsave_t::autosave_mode;
	if(  format == "bzip2"  ) {
		mode = loadsave_t::bzip2;
	}
	else if(  format == "zipped"  ) {
		mode = loadsave_t::zipped;
	}
#if USE_ZSTD
	else if(  format == "zstd"  ) {
		mode = loadsave_t::zstd;
	}
#endif
	else if(  !format.empty()  &&  format != "null"  ) {
		return text_content("{\"error\":\"invalid: format must be bzip2, zipped or zstd (if built with it)\"}");
	}
	// relative to the user directory, as the other savegames
	const std::string file = "save/webview/" + name;

	std::string out = "{";
	live_state::add_head(out, welt);

	// a save of the same world within max_age seconds is given again
	struct stat st;
	auto cached = saved_worlds.find(name);
	if(  cached != saved_worlds.end()  &&  cached->second.epoch == live_changes_t::get_epoch()  &&  max_age > 0
		&&  (sint64)(dr_time() - cached->second.ms) < max_age * 1000  &&  dr_stat((env_t::user_dir + file).c_str(), &st) == 0  ) {
		out += cached->second.json;
		out += ",\"cached\":true,\"age_ms\":";
		out += std::to_string(dr_time() - cached->second.ms);
		out += '}';
		return text_content(out);
	}

	// in a network game the server saves on its own: saving must not change the game
	if(  env_t::networkmode  &&  welt->needs_rotation_to_save()  ) {
		return text_content("{\"error\":\"invalid: this map can only be saved after rotating it, which would change the game of the server only\"}");
	}

	dr_chdir(env_t::user_dir);
	dr_mkdir("save");
	dr_mkdir("save/webview");
	const uint32 sync = welt->get_sync_steps();
	const loadsave_t::mode_t old_mode = loadsave_t::autosave_mode;
	loadsave_t::set_autosavemode(mode);
	const time_t started = time(NULL);
	const uint32 start_ms = dr_time();
	// karte_t::save writes a temporary file (the last letter '_') and renames it, records the event "saved"
	welt->save(file.c_str(), true, SAVEGAME_VER_NR, true);
	const uint32 ms = dr_time() - start_ms;
	loadsave_t::set_autosavemode(old_mode);

	if(  dr_stat(file.c_str(), &st) != 0  ||  st.st_mtime + 1 < started  ) {
		return text_content("{\"error\":\"invalid: the game could not be saved (see the log of the game)\"}");
	}
	std::string fields = ",\"ok\":true,\"path\":";
	fields += jstr("webview/" + name);
	fields += ",\"file\":";
	fields += jstr(file);
	fields += ",\"saved_sync\":";
	fields += std::to_string(sync);
	fields += ",\"seq\":";
	fields += std::to_string(live_changes_t::get_next_seq());
	fields += ",\"bytes\":";
	fields += std::to_string((long long)st.st_size);
	fields += ",\"ms\":";
	fields += std::to_string(ms);
	saved_world_t &entry = saved_worlds[name];
	entry.ms = dr_time();
	entry.epoch = live_changes_t::get_epoch();
	entry.json = fields;

	out += fields;
	out += ",\"cached\":false}";
	return text_content(out);
}


static std::string tool_capture_screen()
{
	std::string png_data;
	const scr_rect screen_area(0, 0, display_get_width(), display_get_height());
	if (!display_snapshot_png(screen_area, png_data)) {
		return text_content("{\"error\":\"failed to capture the Simutrans screen\"}");
	}

	return "{\"content\":[{\"type\":\"image\",\"data\":"
		+ jstr(base64_encode(png_data))
		+ ",\"mimeType\":\"image/png\"}]}";
}

// Renders the whole minimap (entire world, not just the visible viewport) to
// a PNG file on disk, using the same bounded-memory strip export the "Export
// map image" button in the map window uses. The map can be far larger than
// the screen, so unlike capture_screen this does not embed the image inline.
// The optional "mapmode" selects the map mode for this export only; the mode
// of the map window is left unchanged.
static std::string tool_export_minimap(karte_t *welt, const std::string &args_json)
{
	if (!welt) {
		return text_content("{\"error\":\"world not ready\"}");
	}

	std::string mapmode = mcp_server_t::json_get_raw(args_json, "mapmode");
	if (mapmode == "null") {
		mapmode.clear();
	}
	else if (!mapmode.empty() && mapmode[0] == '"') {
		mapmode = mcp_server_t::json_get_string(args_json, "mapmode");
	}
	minimap_t::MAP_DISPLAY_MODE export_mode;
	if (!minimap_t::parse_display_mode(mapmode.c_str(), export_mode)) {
		return text_content("{\"error\":\"invalid mapmode: " + jesc(mapmode) + "\"}");
	}

	std::string filename;
	if (!minimap_t::get_instance()->export_to_png(filename, export_mode)) {
		return text_content("{\"error\":\"failed to export minimap\"}");
	}
	// exclusive modes still export, but the image may lack some of them
	std::string conflicts;
	if (!minimap_t::check_display_mode(export_mode, conflicts)) {
		return text_content("{\"result\":" + jstr(filename) + ",\"warning\":" + jstr("you set exclusive map options: " + conflicts) + "}");
	}
	return text_content("{\"result\":" + jstr(filename) + "}");
}


// ---------------------------------------------------------------------------
// Tool registry
// ---------------------------------------------------------------------------

struct tool_def_t {
	const char *name;
	const char *description;
	const char *input_schema_json;
};

static const tool_def_t TOOL_DEFS[] = {
	{
		"run_squirrel",
		"Execute Squirrel script code with the full Simutrans scripting API "
		"(world, player, halt, line, convoy, factory, command_x, etc.) available. "
		"The script should return a value; it is returned as a string result. "
		"Each call runs in a fresh VM; no state persists between calls.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"code\":{\"type\":\"string\","
		              "\"description\":\"Squirrel script code to execute. "
		              "Use 'return' to pass a value back.\"},"
		   "\"player_nr\":{\"type\":\"integer\","
		                   "\"description\":\"Player context index (0-15), default 1\"}"
		 "},"
		 "\"required\":[\"code\"]}"
	},
	{
		"capture_screen",
		"Capture the current Simutrans window as a PNG image. "
		"Use this to inspect the actual on-screen game view and GUI state.",
		"{\"type\":\"object\",\"properties\":{},\"additionalProperties\":false}"
	},
	{
		"export_minimap",
		"Render the entire minimap (the whole world, not just what fits on screen) "
		"to a PNG file in the screenshot folder and return its path. "
		"Use this to inspect the full map layout rather than the current viewport. "
		"The map mode of the map window is not changed.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"mapmode\":{\"type\":[\"string\",\"integer\"],"
		                 "\"description\":\"optional map mode for this export; default is the map window's mode. "
		                 "Either a bit mask or names joined by '|', e.g. 'MAP_TOWN|MAP_LINES'. "
		                 "Exclusive modes (e.g. two of PASSENGER/MAIL/FREIGHT/TRAFFIC/TRACKS/SPEEDLIMIT/POWERLINES/FOREST/OWNER/LEVEL, "
		                 "two halt modes STATUS/SERVICE/WAITING/WAITCHANGE/ORIGIN/TRANSFER, or such a base mode with "
		                 "HIDE_CONTOUR/CLIMATES) still export, but a 'warning' is returned. Names (MAP_ prefix optional, "
		                 "case insensitive): PLAIN, TOWN, PASSENGER, MAIL, FREIGHT, STATUS, SERVICE, TRAFFIC, ORIGIN, "
		                 "TRANSFER, WAITING, TRACKS, SPEEDLIMIT, POWERLINES, TOURIST, FACTORIES, DEPOT, FOREST, CITYLIMIT, "
		                 "PAX_DEST, OWNER, LINES, LEVEL, WAITCHANGE, HIDE_CONTOUR, CLIMATES, CITIZENS, CITY_GROWTH, LABELS\"}"
		 "},"
		 "\"additionalProperties\":false}"
	},
	{
		"get_live_state",
		"Return every convoy and vehicle as it is now, as JSON: "
		"{schema, epoch, sync, date, ticks, ticks_per_month, pace, time_multiplier, snowline, fps, ahead, paused, "
		"vf, fields, cf, c, gone, full, seq, gen_ms}. "
		"c is a list of [id, owner, state, speed_kmh, reversed, rt0, rt, vehicles, rlap (, tl)] (names in cf); rt is the whole route "
		"as a flat list [x,y,z, ...] starting at route index rt0 (always 0); vehicles (in savegame order) are "
		"lists of the fields named in vf. fields lists every field name that can be requested. "
		"rlap grows whenever the convoy starts its route again or the route changes. "
		"With since (the seq of a previous answer with the same arguments), only what changed since then is returned "
		"(gone: convoys that disappeared; full: true if everything was returned anyway).",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"fields\":{\"type\":\"array\",\"items\":{\"type\":\"string\"},"
		              "\"description\":\"vehicle fields in this order; default "
		              "[x,y,z,dir,steps,steps_next,ri,load,xoff,yoff,air]\"},"
		   "\"since\":{\"type\":[\"integer\",\"null\"],"
		              "\"description\":\"seq of a previous answer: only convoys and records that changed since\"},"
		   "\"routes\":{\"type\":\"boolean\","
		              "\"description\":\"false: rt is null for every convoy (default true)\"},"
		   "\"route_ids\":{\"type\":\"array\",\"items\":{\"type\":\"integer\"},"
		                 "\"description\":\"only these convoys get rt, the others null\"},"
		   "\"route_hash\":{\"type\":\"boolean\","
		                  "\"description\":\"also return rh: a hash of each returned convoy's route, in the order of c\"},"
		   "\"tile_lens\":{\"type\":\"boolean\","
		                 "\"description\":\"add tl to every convoy: the length in steps of each tile of rt (null without rt)\"},"
		   "\"halts\":{\"type\":\"boolean\","
		             "\"description\":\"also return halts: [[id, owner, waiting total, "
		             "[[via halt id, goods index, amount], ...], status 0-3]] and goods (the goods names by index)\"},"
		   "\"factories\":{\"type\":\"boolean\"},"
		   "\"cities\":{\"type\":\"boolean\"},"
		   "\"players\":{\"type\":\"boolean\"},"
		   "\"depots\":{\"type\":\"boolean\"},"
		   "\"lines\":{\"type\":\"boolean\",\"description\":\"lines and the schedules of the convoys\"}"
		 "},"
		 "\"additionalProperties\":false}"
	},
	{
		"get_tiles",
		"Return a rectangle of tiles exactly as the savegame stores them: "
		"{epoch, sync, version, otrp, x0, y0, w, h, th, data}. data (base64) holds the tiles row by row "
		"(y outside, x inside), each as the savegame writes it, followed by the w*h climate bytes; th is a hash (FNV-1a) of data.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"x0\":{\"type\":\"integer\"},"
		   "\"y0\":{\"type\":\"integer\"},"
		   "\"w\":{\"type\":\"integer\"},"
		   "\"h\":{\"type\":\"integer\",\"description\":\"w*h must not exceed 4096\"}"
		 "},"
		 "\"required\":[\"x0\",\"y0\",\"w\",\"h\"],"
		 "\"additionalProperties\":false}"
	},
	{
		"get_changes",
		"Return the changes of the static world since a sequence number: "
		"{epoch, sync, seq, floor, overflow, tiles:[[sync,x,y,kind],...], sig:[[sync,x,y,z,typ,dir,state],...], "
		"events:[{type:sold, sync, convoy, player, amount}, ...]}. "
		"kind: bits of what changed on the tile: 1 road, 2 track, 4 water way, 8 air way, 16 signal or sign, "
		"32 building, 64 station, depot or stop name, 128 terrain, 256 other. "
		"Pass the seq of the previous answer as since (null at first: then only seq is returned). "
		"overflow is true when entries since then are lost (reload the whole world). "
		"Changes are recorded only while the MCP server runs.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"since\":{\"type\":[\"integer\",\"null\"]}"
		 "},"
		 "\"additionalProperties\":false}"
	},
	{
		"dump_vehicles",
		"Return all fields of the given convoys and their vehicles by name, with the whole route (for diagnosis).",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"ids\":{\"type\":\"array\",\"items\":{\"type\":\"integer\"},"
		           "\"description\":\"convoy ids (at most 50)\"}"
		 "},"
		 "\"required\":[\"ids\"],"
		 "\"additionalProperties\":false}"
	},
	{
		"get_info",
		"Return the basic information of halts, convoys, lines, factories, cities, players or depots as rows "
		"(the last value of a row is its hash, FNV-1a of the row without the current stop of a schedule): "
		"{epoch, sync, ..., kind, level, hash_only, rows, gone (ids asked for that do not exist), vnames (convoys)}. "
		"halts: [id, name, owner, [[x,y,z],...], station type bits, enabled bits (1 passengers 2 mail 4 goods), "
		"[other player connection, no passengers, no mail, no goods, allow unload longer convoy, permission bits], hash]; "
		"convoys: [id, name, owner, line (-1), state, home depot [x,y,z], [vehicle types (vnames index)], schedule, vtype (obj_t::typ of the front vehicle, -1), "
		"[no load, unload all, suspension, delay recovery, max speed km/h, balance speed, invalid, permit trade, player offered to (0 none)], hash]; "
		"lines: [id, name, owner, line type, colour, schedule, [convoy ids], [memo, withdraw], hash]; "
		"factories: [x, y, z, name, custom name, owner, base production, [[field x, y, class],...], [input goods], [output goods], [no close, shipment size], hash]; "
		"cities: [x, y, name, population, [lo x, lo y, ur x, ur y], bev, arb, won, hash]; "
		"players: [nr, name, colour 1, colour 2, ai type, locked, hash]; depots: [x, y, z, owner, waytype, name, hash]. "
		"schedule: [current, flags, max speed, base waiting time, next line (-1), [[x,y,z,stop flags,min load,waiting time shift],...]]; "
		"level full adds the departure slot group line and the waytype before the entries, and every other entry field. "
		"level area (halts): [id, [[y, x0, x1], ...], hash], the tiles the halt serves. "
		"kind settings: {..., kind, settings: {name: value}} with the settings the checks of the tools depend on.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"kind\":{\"type\":\"string\",\"enum\":[\"halts\",\"convoys\",\"lines\",\"factories\",\"cities\",\"players\",\"depots\",\"settings\"]},"
		   "\"ids\":{\"type\":\"array\",\"description\":\"ids, or [x,y] / [x,y,z] for factories, cities, depots; default all\"},"
		   "\"level\":{\"type\":\"string\",\"enum\":[\"basic\",\"full\",\"area\"]},"
		   "\"hash_only\":{\"type\":\"boolean\",\"description\":\"rows are [key..., hash] only\"}"
		 "},"
		 "\"required\":[\"kind\"],"
		 "\"additionalProperties\":false}"
	},
	{
		"get_history",
		"Return the statistics as the savegame stores them (month or year 0 is the current one): "
		"{epoch, sync, ..., kind, month (month counter), rows, gone}. halts [id, [cost][month]], convoys [id, [cost][month], total distance], "
		"lines [id, [cost][month]], cities [x, y, [year][type], [month][type]], "
		"factories [x, y, z, [type][month], [[type][month] per input], [[type][month] per output]], "
		"players [nr, [year][type], [month][type], [transport type][year][type], [transport type][month][type]], "
		"world: one row [[year][type], [month][type], [decade][type]].",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"kind\":{\"type\":\"string\",\"enum\":[\"halts\",\"convoys\",\"lines\",\"cities\",\"factories\",\"players\",\"world\"]},"
		   "\"ids\":{\"type\":\"array\",\"description\":\"as for get_info; default all\"}"
		 "},"
		 "\"required\":[\"kind\"],"
		 "\"additionalProperties\":false}"
	},
	{
		"get_messages",
		"Return the messages of the game recorded while the MCP server runs (the last 1024): "
		"{epoch, sync, ..., seq, floor, overflow, messages:[{seq, sync, month, type, text, player (-1 none), x, y}]}. "
		"Pass the seq of the previous answer as since; without since every message kept is returned.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"since\":{\"type\":[\"integer\",\"null\"]}"
		 "},"
		 "\"additionalProperties\":false}"
	},
	{
		"capture_view",
		"Render the world (without windows) centred on a tile at a zoom step and return it as PNG, "
		"followed by a text with the parameters used. The size is limited to the game window. "
		"The view of the player is restored afterwards.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"x\":{\"type\":\"integer\"},"
		   "\"y\":{\"type\":\"integer\"},"
		   "\"z\":{\"type\":\"integer\",\"description\":\"default: the height of the ground\"},"
		   "\"zoom\":{\"type\":\"integer\",\"description\":\"zoom step 0 (largest) to 9; 3 is 1:1; default: the current one\"},"
		   "\"w\":{\"type\":\"integer\"},"
		   "\"h\":{\"type\":\"integer\"}"
		 "},"
		 "\"required\":[\"x\",\"y\"],"
		 "\"additionalProperties\":false}"
	},
	{
		"save_world",
		"Write the world as it is now as a savegame (as an autosave, silently) into the folder save/webview/ "
		"of the user directory of the game, and answer when it is written: "
		"{epoch, sync, ..., ok, path (webview/<name>), file (save/webview/<name>), saved_sync, "
		"seq (get_changes since this seq gives what changed after the save), bytes, ms (time taken), cached, age_ms}. "
		"The game stops while it saves (other requests are answered busy). "
		"A save of the same name within max_age seconds is not written again: its answer is given again (cached true). "
		"In a network game only this game saves; the clients are not synchronised.",
		"{\"type\":\"object\","
		 "\"properties\":{"
		   "\"name\":{\"type\":\"string\",\"description\":\"file name: letters, digits and _-., ending with .sve, at most 64 bytes\"},"
		   "\"max_age\":{\"type\":\"integer\",\"description\":\"seconds (default 30, 0: always write)\"},"
		   "\"format\":{\"type\":\"string\",\"enum\":[\"bzip2\",\"zipped\",\"zstd\"],"
		              "\"description\":\"default: the autosave format of the game (simuconf.tab autosaveformat)\"}"
		 "},"
		 "\"required\":[\"name\"],"
		 "\"additionalProperties\":false}"
	}
};
static const int NUM_TOOLS = sizeof(TOOL_DEFS) / sizeof(TOOL_DEFS[0]);


// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

std::string mcp_tools::tools_list_json()
{
	std::string arr = "[";
	for (int i = 0; i < NUM_TOOLS; i++) {
		if (i > 0) arr += ',';
		arr += std::string("{")
			+ "\"name\":"        + jstr(TOOL_DEFS[i].name)        + ","
			+ "\"description\":" + jstr(TOOL_DEFS[i].description) + ","
			+ "\"inputSchema\":"  + TOOL_DEFS[i].input_schema_json
			+ "}";
	}
	return arr + "]";
}


std::string mcp_tools::tools_call(const std::string &name,
                                   const std::string &args_json,
                                   karte_t           *welt,
                                   script_vm_t      **out_pending_vm)
{
	if (name == "run_squirrel") {
		std::string code     = mcp_server_t::json_get_string(args_json, "code");
		std::string player_s = mcp_server_t::json_get_raw(args_json, "player_nr");
		int player_nr = player_s.empty() ? 1 : atoi(player_s.c_str());
		if (player_nr < 0 || player_nr > MAX_PLAYER_COUNT-1) player_nr = 1;
		script_vm_t *pending_vm = nullptr;
		std::string result = tool_run_squirrel(code, player_nr, &pending_vm);
		if (out_pending_vm) {
			*out_pending_vm = pending_vm;
		}
		else if (pending_vm) {
			// Caller doesn't support async — treat as error
			delete pending_vm;
			return text_content("{\"error\":\"script suspended but caller does not support async\"}");
		}
		return text_content(result);
	}

	if (name == "capture_screen") {
		return tool_capture_screen();
	}

	if (name == "export_minimap") {
		return tool_export_minimap(welt, args_json);
	}

	if (name == "get_live_state") {
		return text_content(live_state::get_live_state(welt, args_json));
	}

	if (name == "get_tiles") {
		return text_content(live_state::get_tiles(welt, args_json));
	}

	if (name == "get_changes") {
		return text_content(live_state::get_changes(welt, args_json));
	}

	if (name == "dump_vehicles") {
		return text_content(live_state::dump_vehicles(welt, args_json));
	}

	if (name == "get_info") {
		return text_content(live_state::get_info(welt, args_json));
	}

	if (name == "get_history") {
		return text_content(live_state::get_history(welt, args_json));
	}

	if (name == "get_messages") {
		return text_content(live_state::get_messages(welt, args_json));
	}

	if (name == "capture_view") {
		return tool_capture_view(welt, args_json);
	}

	if (name == "save_world") {
		// mcp_server_t calls it after the requests of a step
		return save_world(welt, args_json);
	}

	return text_content("{\"error\":\"unknown tool: " + jesc(name) + "\"}");
}
