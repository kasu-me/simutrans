/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_FREIGHT_LIST_H
#define GUI_COMPONENTS_GUI_FREIGHT_LIST_H


#include "gui_aligned_container.h"
#include "action_listener.h"
#include "gui_speedbar.h"
#include "../../halthandle_t.h"
#include "../../simware.h"
#include "../../simcolor.h"
#include "../../tpl/vector_tpl.h"
#include "../../display/scr_coord.h"

class button_t;


/**
 * Small marker: round for the color of a player, square for the color of a goods category
 */
class gui_color_dot_t : public gui_component_t
{
	PIXVAL color;
	bool square;
public:
	gui_color_dot_t(PIXVAL c, bool sq = false) : color(c), square(sq) {}

	void draw(scr_coord offset) OVERRIDE;

	scr_size get_min_size() const OVERRIDE;

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


/**
 * List of goods (waiting at a stop or loaded in a convoy), grouped by goods category.
 * Each category has a heading with amount, capacity and a bar; the entries are sorted
 * like freight_list_sorter_t does. Entries merged by their next stop can be expanded
 * to show their final destinations.
 *
 * The list is rebuilt when the goods change, but at most twice per second
 * and not while the mouse is over it (so the entries do not move away under the mouse).
 */
class gui_freight_list_t : public gui_aligned_container_t, private action_listener_t
{
	// state of the last rebuild
	uint8 last_sortby;
	bool rebuild_pending;
	bool rebuild_at_once;
	uint32 last_rebuild_time;

	// visible part of the list on screen at the last draw; empty, if it was not drawn
	scr_rect visible_area;

	// entries merged by their next stop, which show their final destinations; key from expand_key()
	vector_tpl<uint32> expanded;
	// the buttons to expand/collapse the entries, and their keys
	vector_tpl<button_t *> expand_buttons;
	vector_tpl<uint32> expand_button_keys;

	// tables added during a rebuild
	vector_tpl<gui_container_t *> new_tables;

	// number of categories added during a rebuild
	uint32 category_count;

	static uint32 expand_key(const ware_t &ware) { return ((uint32)ware.get_index() << 16) | ware.get_zwischenziel().get_id(); }

	/// adds the final destinations of the goods merged into the entry @p entry
	void add_final_destinations(const vector_tpl<ware_t> &wares, const ware_t &entry);

	bool action_triggered(gui_action_creator_t *comp, value_t) OVERRIDE;

	void rebuild();

protected:
	/// add_table(), remembering the table for rebuild()
	gui_aligned_container_t *add_list_table(uint16 columns, uint16 rows);

	/// @returns true, if there is something (a halt, a convoy) to show
	virtual bool has_source() const = 0;

	/// sort mode (freight_list_sorter_t::sort_mode_t) of the source
	virtual uint8 get_sortby() const = 0;

	/// @returns true, if the shown data changed since the last call
	virtual bool is_rebuild_needed() = 0;

	/// adds all components to the emptied list, mainly by add_category()
	virtual void fill() = 0;

	/// bit field of the players serving @p next_halt, shown as dots behind each entry
	virtual uint64 get_serving_players(halthandle_t) { return 0; }

	/// adds a dot for each player in @p players
	void add_player_dots(uint64 players);

	/**
	 * Adds the heading and the entries of a goods category.
	 * @param wares all goods of this category
	 * @param total sum of the amounts of @p wares
	 * @param capacity the bar is only shown for a capacity >0
	 * @param overload_capacity capacity including overloading; shown in addition, if larger than @p capacity
	 */
	void add_category(uint8 catg_index, const vector_tpl<ware_t> &wares, uint64 total, uint32 capacity, uint32 overload_capacity = 0);

	/// forget the expanded entries and rebuild on the next update(), i.e. after the source changed
	void reset();

public:
	gui_freight_list_t();

	/// rebuilds the list if needed; @returns true, if it was rebuilt (and its size may have changed)
	bool update();

	void draw(scr_coord offset) OVERRIDE;
};

#endif
