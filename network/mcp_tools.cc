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
 * get_live_state, get_tiles, get_changes and dump_vehicles are the read-only
 * tools for a live web view of the game (see live_state.cc).
 */

#include "mcp_tools.h"
#include "mcp_server.h"
#include "live_state.h"

#include <stdio.h>
#include <string.h>
#include <string>

#include "../script/script.h"
#include "../script/script_loader.h"
#include "../dataobj/environment.h"
#include "../display/simgraph.h"
#include "../utils/cbuffer_t.h"
#include "../utils/plainstring.h"
#include "../simconst.h"
#include "../simworld.h"
#include "../gui/minimap.h"


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
		// Script suspended waiting for a network command — hand VM to caller
		*out_vm = vm;
		return "";
	}

	// done at once: the commands took effect in the current sync step
	const std::string sync = world() ? std::to_string(world()->get_sync_steps()) : "0";
	const std::string syncs = ",\"sync_start\":" + sync + ",\"sync\":" + sync + "}";
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
		"{epoch, sync, seq, floor, overflow, tiles:[[sync,x,y],...], sig:[[sync,x,y,z,typ,dir,state],...], "
		"events:[{type:sold, sync, convoy, player, amount}, ...]}. "
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

	return text_content("{\"error\":\"unknown tool: " + jesc(name) + "\"}");
}
