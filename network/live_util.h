/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef NETWORK_LIVE_UTIL_H
#define NETWORK_LIVE_UTIL_H


#include "../simtypes.h"

#include <string>
#include <unordered_map>
#include <vector>

class karte_t;
class vehicle_desc_t;


/**
 * Helpers shared by live_state.cc and live_world.cc (the read-only MCP tools
 * for a live view of the game): JSON output and the change detection for the
 * `since` argument.
 */
namespace live_util {

void add_int(std::string &out, sint64 v);
void add_double(std::string &out, double v, int decimals);
void add_str(std::string &out, const char *s);

std::string error_json(const char *msg);

/// an integer argument; false if missing or not a number
bool get_int_arg(const std::string &args_json, const char *key, sint64 &value);

/// true if the argument is the JSON literal true
bool get_bool_arg(const std::string &args_json, const char *key);

uint32 fnv1a(const char *data, size_t len, uint32 h = 2166136261u);
inline uint32 fnv1a(const std::string &s) { return fnv1a(s.data(), s.size()); }

/// index of a vehicle type in the table "vnames" (append-only while the game runs)
uint32 vehicle_name_index(const vehicle_desc_t *desc);

/// "vnames":[...] (without leading comma)
void add_vehicle_names(std::string &out);

/**
 * The answers with `since` contain only the entities that changed since the
 * caller's previous answer. Each call computes the record of every entity,
 * and an entity whose record differs from the one of the previous call gets
 * a new sequence number (one counter for all kinds). Nothing has to be marked
 * where the game changes things, so no change can be missed.
 */
class entity_changes_t
{
public:
	/// starts a call; a full answer is given unless `since` is known to this cache
	void begin(bool has_since, uint32 since);

	/// @returns true if the entity is to be sent (changed since `since`, or a full answer)
	bool update(uint64 key, const std::string &record);

	/// the keys of the entities that disappeared since `since` (none for a full answer); call after all update()
	std::vector<uint64> end();

	bool is_full() const { return full; }

	/// forgets everything (another world)
	void clear();

	/// current value of the counter: the `seq` of the answer
	static uint32 get_seq() { return seq; }

private:
	struct entry_t {
		uint32 hash;
		uint32 seq;     ///< when the record last changed (or the entity disappeared)
		uint32 seen;    ///< number of the call that saw it last
		bool gone;
	};
	std::unordered_map<uint64, entry_t> entries;
	uint32 calls = 0;
	uint32 floor = 0;   ///< answers for an older `since` are full (forgotten tombstones)
	bool full = true;
	uint32 since = 0;

	static uint32 seq;
};

/// one cache per kind (and per shape of the convoy records), all forgotten with the world
entity_changes_t &changes_of(const std::string &kind);
void clear_all_changes();

/**
 * The world sections of get_live_state asked for in the arguments (halts,
 * factories, cities, players, depots, lines), each with a leading comma.
 * @param need_vnames set when a section refers to the table "vnames"
 */
void add_world_sections(std::string &out, karte_t *welt, const std::string &args_json, bool has_since, uint32 since, bool &need_vnames);

}

#endif
