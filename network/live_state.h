/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_LIVE_STATE_H
#define NETWORK_LIVE_STATE_H


#include "../simtypes.h"
#include "../simconst.h"
#include "../dataobj/koord3d.h"

#include <string>
#include <vector>

class karte_t;
class obj_t;
class player_t;


/**
 * Change log of the static world for the MCP tool get_changes.
 *
 * The hooks in the game code (object list, grounds, terrain, ways, buildings,
 * signals, crossings) report the tiles they modify. While the MCP server is
 * listening, the tiles and signal states are kept in a ring buffer, numbered
 * with a sequence number. Nothing is recorded while a game is loaded or
 * generated, nor when the MCP server is off (then each hook costs only a test
 * of a static flag).
 *
 * The epoch identifies the world: it changes when another game is loaded or
 * generated, when the map is rotated or enlarged, but not when the very same
 * game is reloaded for a client joining a network game.
 */
class live_changes_t
{
public:
	/// true while changes are to be recorded
	static bool recording;

	/// what changed on a tile (the kind bits of the tiles of get_changes; several when merged)
	enum {
		T_ROAD     = 1 << 0, ///< road
		T_TRACK    = 1 << 1, ///< rail, tram, monorail, maglev, narrow gauge
		T_WATER    = 1 << 2, ///< canal, river
		T_AIR      = 1 << 3, ///< runway, taxiway
		T_SIGN     = 1 << 4, ///< signal, roadsign (added, removed, turned, settings; not its state)
		T_BUILDING = 1 << 5, ///< a building other than T_STATION (houses, factories, monuments ...)
		T_STATION  = 1 << 6, ///< a stop, depot or other transport building, the name of a stop
		T_TERRAIN  = 1 << 7, ///< ground: height, slope, water, climate, bridges' and tunnels' grounds
		T_OTHER    = 1 << 8  ///< everything else (trees, power lines, way objects, labels, texts ...)
	};

	/// the kind of a way of this waytype (T_ROAD ... T_AIR, else T_OTHER)
	static uint16 way_kind(waytype_t wt);

	/// a tile changed; kind: T_ROAD ... T_OTHER
	static void tile(sint16 x, sint16 y, uint16 kind) { if(  recording  ) { record_tile(x, y, kind); } }

	/// a static object was added to or removed from a tile, or changed in place
	static void obj(const obj_t *o) { if(  recording  ) { record_obj(o, true); } }

	/// as obj(), but the object is not asked for its type (it may be under construction): T_OTHER unless known otherwise
	static void obj_untyped(const obj_t *o) { if(  recording  ) { record_obj(o, false); } }

	/**
	 * A static object is being destroyed (from the destructor of its class: obj_t::~obj_t
	 * removes it from its tile later, when it cannot tell its type any more).
	 */
	static void obj_destroyed(const obj_t *o, uint16 kind);

	/// marks the object whose obj_t::~obj_t runs: it must not be asked for its type
	class destroying_t
	{
	public:
		explicit destroying_t(const obj_t *o) : outer(destroying) { destroying = o; }
		~destroying_t() { destroying = outer; }
	private:
		const obj_t *outer;
	};

	/// a signal or roadsign (typ, ribi dir) or a crossing (typ, dir 0) changed its state
	static void sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state) { if(  recording  ) { record_sign(o, typ, dir, state); } }

	/// a convoy was sold (scrapped): the money (1/100 credits) is booked now (event "sold" of get_changes)
	static void sold(uint32 convoy, uint8 player_nr, sint64 amount) { if(  recording  ) { record_sold(convoy, player_nr, amount); } }

	/// the leading vehicle of a convoy starts driving its route from the start (the "rlap" of get_live_state)
	static void route_restarted(uint32 convoy) { if(  recording  ) { route_restarted_intern(convoy); } }

	/// kinds of the entity events of get_changes (named in the event's "type")
	enum entity_kind_t { K_STATION, K_CONVOY, K_LINE, K_FACTORY, K_CITY, K_PLAYER, K_DEPOT, K_MAX };

	/// what changed (the event's "what", several at once when merged)
	enum {
		W_NEW      = 1 << 0,
		W_GONE     = 1 << 1,
		W_NAME     = 1 << 2,
		W_TILES    = 1 << 3,
		W_LINE     = 1 << 4,
		W_SCHEDULE = 1 << 5,
		W_VEHICLES = 1 << 6,
		W_COLOUR   = 1 << 7,
		W_FIELDS   = 1 << 8,
		W_SETTINGS = 1 << 9, ///< a setting of the windows (no load, suspension, memo, permissions, shipment size ...)
		W_MAX_BIT  = 10
	};

	/// an entity identified by its id (station, convoy, line, player) changed
	static void entity(entity_kind_t kind, uint32 id, uint16 what) { if(  recording  ) { record_entity(kind, id, koord3d::invalid, what); } }

	/// an entity identified by its position (factory, city, depot) changed
	static void entity_at(entity_kind_t kind, koord3d pos, uint16 what) { if(  recording  ) { record_entity(kind, 0, pos, what); } }

	/// a savegame was written (event "saved")
	static void saved(const char *filename) { if(  recording  ) { record_saved(filename); } }

	/// a new month began (event "month")
	static void new_month() { if(  recording  ) { record_month(); } }

	/// a message was added to the message list (get_messages)
	static void message(const char *text, sint32 type, koord pos, uint32 color) { if(  recording  ) { record_message(text, type, pos, color); } }

	/**
	 * A tool executed for a script (network game: nwc_tool_t::do_command; else at once).
	 * The cost, the tiles changed and the events are kept for the MCP call that ran the
	 * script (see take_tool_results).
	 */
	class tool_op_t
	{
	public:
		explicit tool_op_t(const player_t *player);
		/// @param vm the script vm (HSQUIRRELVM) that called the tool
		void done(const void *vm, uint16 tool_id, const char *default_param, const char *err);
	private:
		const player_t *player;
		sint64 balance[MAX_PLAYER_COUNT]; ///< of every player: a tool may book to another player (a depot's owner)
		uint32 seq;
		bool active;
	};

	/// the results of the tools that ran for the script vms that match, as a JSON array ("[...]"), removed from the list
	template<class MATCH> static std::string take_tool_results(MATCH match)
	{
		std::string out = "[";
		for(  size_t i = 0;  i < tool_results.size();  ) {
			if(  match(tool_results[i].vm)  ) {
				if(  out.size() > 1  ) {
					out += ',';
				}
				out += tool_results[i].json;
				tool_results.erase(tool_results.begin() + i);
			}
			else {
				i++;
			}
		}
		return out + "]";
	}

	/**
	 * While a game is saved, loaded or sent to a joining client, the main loop does not run:
	 * busy_t marks such a section, and busy_poll() (called from its loops) answers the MCP
	 * clients with a "busy" error instead of leaving them waiting.
	 */
	class busy_t
	{
	public:
		explicit busy_t(const char *reason);
		~busy_t();
	private:
		const char *outer;
	};

	/// in a busy section: answer the MCP clients (at most every few milliseconds)
	static void busy_poll() { if(  busy_reason  &&  enabled  ) { busy_poll_intern(); } }

	/// the innermost busy section, NULL outside
	static const char *get_busy_reason() { return busy_reason; }

	/// true while a client joins (saving, sending and reloading the game)
	static bool is_joining() { return joining > 0; }

	/// the MCP server started or stopped listening
	static void set_enabled(bool yesno);

	/**
	 * Stops recording for its lifetime (loading, generating, rotating a map),
	 * and starts a new epoch at the end, unless keep_epoch_on_next_load() was
	 * called before.
	 */
	class world_reset_t
	{
	public:
		world_reset_t();
		~world_reset_t();
	};

	/// the next load reloads the same world (network sync for a joining client)
	static void keep_epoch_on_next_load() { keep_epoch = true; }

	static uint32 get_epoch();

	/// the sequence number the next entry of the change log gets (the "seq" of get_changes)
	static uint32 get_next_seq();

	/**
	 * Builds the JSON object of get_changes.
	 * @param has_since false for the first call (only the current seq is returned)
	 */
	static std::string get_changes_json(karte_t *welt, bool has_since, uint32 since);

private:
	/// kind 0: what changed is not known (T_OTHER, unless the tile gets a kind in the same step)
	static void record_tile(sint16 x, sint16 y, uint16 kind);
	static void record_obj(const obj_t *o, bool typed);
	static void record_sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state);
	static void record_sold(uint32 convoy, uint8 player_nr, sint64 amount);
	static void record_entity(entity_kind_t kind, uint32 id, koord3d pos, uint16 what);
	static void record_saved(const char *filename);
	static void record_month();
	static void record_message(const char *text, sint32 type, koord pos, uint32 color);
	static void route_restarted_intern(uint32 convoy);
	static void busy_poll_intern();
	static void update_recording();
	/// another world: forgets what the answers of get_live_state compare with
	static void forget_world();

	struct tool_result_t {
		const void *vm;
		std::string json;
	};
	static std::vector<tool_result_t> tool_results;

	static const obj_t *destroying;
	static bool enabled;
	static uint32 world_resets;
	static bool keep_epoch;
	static uint32 epoch;
	static const char *busy_reason;
	static int joining;
};


/**
 * Read-only MCP tools for the live view of the game: get_live_state,
 * get_tiles, get_changes, dump_vehicles, get_info, get_history and get_messages.
 * Each returns the JSON object that goes into the text content of the result.
 */
namespace live_state {

std::string get_live_state(karte_t *welt, const std::string &args_json);

std::string get_tiles(karte_t *welt, const std::string &args_json);

std::string get_changes(karte_t *welt, const std::string &args_json);

std::string dump_vehicles(karte_t *welt, const std::string &args_json);

/// the basic information of stations, convoys, lines, factories, cities, players, depots (live_info.cc)
std::string get_info(karte_t *welt, const std::string &args_json);

/// the monthly and yearly statistics, as the savegame stores them (live_info.cc)
std::string get_history(karte_t *welt, const std::string &args_json);

/// the messages of the game since a sequence number
std::string get_messages(karte_t *welt, const std::string &args_json);

/// the head of every answer: epoch, sync, date, ticks and the time of the world, without braces
void add_head(std::string &out, karte_t *welt);

}

#endif
