/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_LIVE_STATE_H
#define NETWORK_LIVE_STATE_H


#include "../simtypes.h"

#include <string>

class karte_t;
class obj_t;


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

	static void tile(sint16 x, sint16 y) { if(  recording  ) { record_tile(x, y); } }

	/// a static object was added to or removed from a tile, or changed in place
	static void obj(const obj_t *o) { if(  recording  ) { record_obj(o); } }

	/// a signal or roadsign (typ, ribi dir) or a crossing (typ, dir 0) changed its state
	static void sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state) { if(  recording  ) { record_sign(o, typ, dir, state); } }

	/// a convoy was sold (scrapped): the money (1/100 credits) is booked now (event "sold" of get_changes)
	static void sold(uint32 convoy, uint8 player_nr, sint64 amount) { if(  recording  ) { record_sold(convoy, player_nr, amount); } }

	/// the leading vehicle of a convoy starts driving its route from the start (the "rlap" of get_live_state)
	static void route_restarted(uint32 convoy) { if(  recording  ) { route_restarted_intern(convoy); } }

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

	/**
	 * Builds the JSON object of get_changes.
	 * @param has_since false for the first call (only the current seq is returned)
	 */
	static std::string get_changes_json(karte_t *welt, bool has_since, uint32 since);

private:
	static void record_tile(sint16 x, sint16 y);
	static void record_obj(const obj_t *o);
	static void record_sign(const obj_t *o, uint8 typ, uint8 dir, uint8 state);
	static void record_sold(uint32 convoy, uint8 player_nr, sint64 amount);
	static void route_restarted_intern(uint32 convoy);
	static void update_recording();
	/// another world: forgets what the answers of get_live_state compare with
	static void forget_world();

	static bool enabled;
	static uint32 world_resets;
	static bool keep_epoch;
	static uint32 epoch;
};


/**
 * Read-only MCP tools for the live view of the game: get_live_state,
 * get_tiles, get_changes and dump_vehicles.
 * Each returns the JSON object that goes into the text content of the result.
 */
namespace live_state {

std::string get_live_state(karte_t *welt, const std::string &args_json);

std::string get_tiles(karte_t *welt, const std::string &args_json);

std::string get_changes(karte_t *welt, const std::string &args_json);

std::string dump_vehicles(karte_t *welt, const std::string &args_json);

}

#endif
