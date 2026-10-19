/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <algorithm>
#include <string.h>

#include "gui_freight_list.h"
#include "gui_button.h"
#include "gui_label.h"
#include "../gui_theme.h"

#include "../../simworld.h"
#include "../../simhalt.h"
#include "../../simfab.h"
#include "../../simevent.h"
#include "../../freight_list_sorter.h"
#include "../../bauer/goods_manager.h"
#include "../../dataobj/environment.h"
#include "../../dataobj/translator.h"
#include "../../display/simgraph.h"
#include "../../player/simplay.h"
#include "../../sys/simsys.h"
#include "../../utils/simstring.h"


void gui_color_dot_t::draw(scr_coord offset)
{
	offset += pos;
	if(  square  ) {
		display_ddd_box_clip_rgb(offset.x, offset.y, size.w, size.h, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
		display_fillbox_wh_clip_rgb(offset.x + 1, offset.y + 1, size.w - 2, size.h - 2, color, true);
		return;
	}
	const scr_coord_val r = max(2, (min(size.w, size.h) - 1) / 2);
	const scr_coord_val cx = offset.x + size.w / 2;
	const scr_coord_val cy = offset.y + size.h / 2;
	display_filled_circle_rgb(cx, cy, r, color_idx_to_rgb(MN_GREY0));
	display_filled_circle_rgb(cx, cy, r - 1, color);
	mark_rect_dirty_wc(cx - r, cy - r, cx + r + 1, cy + r + 1);
}


scr_size gui_color_dot_t::get_min_size() const
{
	const scr_coord_val d = max(9, (LINESPACE * 3) / 4) | 1; // odd, so the dot is centered
	return scr_size(d, d);
}


/**
 * Tree lines connecting a sub entry to its parent entry above
 */
class gui_tree_branch_t : public gui_component_t
{
	bool first, last;
public:
	gui_tree_branch_t(bool first_, bool last_) : first(first_), last(last_) {}

	void draw(scr_coord offset) OVERRIDE
	{
		offset += pos;
		const PIXVAL color = SYSCOL_BUTTON_TEXT_DISABLED;
		const scr_coord_val x = offset.x + LINESPACE / 3;
		const scr_coord_val y_mid = offset.y + size.h / 2;
		// the first one reaches up through the spacing to the parent entry
		const scr_coord_val y_top = first ? offset.y - D_V_SPACE : offset.y;
		display_vline_wh_clip_rgb(x, y_top, (last ? y_mid + 1 : offset.y + size.h) - y_top, color, true);
		display_fillbox_wh_clip_rgb(x, y_mid, offset.x + size.w - x, 1, color, true);
	}

	scr_size get_min_size() const OVERRIDE { return scr_size(LINESPACE, D_LABEL_HEIGHT + 2); } // a little air between the lines

	// fixed height: an inf height would take the free space of the window and stretch the rows
	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


/**
 * Bar showing an amount relative to a capacity
 */
class gui_capacity_bar_t : public gui_speedbar_t
{
	sint32 value;
public:
	gui_capacity_bar_t(sint32 v, sint32 capacity, PIXVAL color) : value(v)
	{
		set_base(capacity);
		add_color_value(&value, color);
	}

	scr_size get_min_size() const OVERRIDE { return scr_size((D_BUTTON_WIDTH * 3) / 2, max(D_INDICATOR_HEIGHT, LINESPACE / 2)); }

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }
};


static const char *get_list_arrow()
{
	return has_character(0x2192) ? "\xE2\x86\x92 " : "> ";
}


gui_freight_list_t::gui_freight_list_t() :
	last_sortby(0xFF),
	rebuild_pending(true),
	rebuild_at_once(false),
	last_rebuild_time(0),
	category_count(0)
{
	set_table_layout(1,0);
}


void gui_freight_list_t::reset()
{
	last_sortby = 0xFF;
	rebuild_pending = true;
	expanded.clear();
}


gui_aligned_container_t *gui_freight_list_t::add_list_table(uint16 columns, uint16 rows)
{
	gui_aligned_container_t *table = add_table(columns, rows);
	new_tables.append(table);
	return table;
}


void gui_freight_list_t::add_player_dots(uint64 players)
{
	for(  uint8 i = 0;  i < MAX_PLAYER_COUNT;  i++  ) {
		if(  players & ((uint64)1 << i)  ) {
			if(  player_t *pl = world()->get_player(i)  ) {
				new_component<gui_color_dot_t>(color_idx_to_rgb(pl->get_player_color1() + env_t::gui_player_color_bright));
			}
		}
	}
}


bool gui_freight_list_t::update()
{
	if(  !has_source()  ) {
		return false;
	}
	// while the mouse is over the list, the entries must not move away under it
	const bool mouse_over = visible_area.w > 0  &&  visible_area.h > 0  &&  visible_area.contains( scr_coord(get_mouse_x(), get_mouse_y()) );
	visible_area = scr_rect();

	rebuild_pending |= is_rebuild_needed();
	rebuild_pending |= last_sortby != get_sortby();
	if(  !rebuild_pending  ) {
		return false;
	}
	// a changed sort order or expanded entry is shown at once, changed amounts at most twice per second
	if(  !rebuild_at_once  &&  last_sortby == get_sortby()  &&  (mouse_over  ||  dr_time() - last_rebuild_time < 500)  ) {
		return false;
	}
	rebuild();
	rebuild_pending = false;
	rebuild_at_once = false;
	last_rebuild_time = dr_time();
	return true;
}


void gui_freight_list_t::draw(scr_coord offset)
{
	// clipped to the scrollpane
	const clip_dimension cr = display_get_clip_wh();
	visible_area = scr_rect( offset + pos, size );
	visible_area.clip( scr_rect(cr.x, cr.y, cr.w, cr.h) );

	gui_aligned_container_t::draw(offset);
}


bool gui_freight_list_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	for(  uint32 i = 0;  i < expand_buttons.get_count();  i++  ) {
		if(  comp == expand_buttons[i]  ) {
			const uint32 key = expand_button_keys[i];
			if(  !expanded.remove(key)  ) {
				expanded.append(key);
			}
			// not here, since this would delete the button which is just calling us
			rebuild_pending = true;
			rebuild_at_once = true;
			return true;
		}
	}
	return false;
}


void gui_freight_list_t::rebuild()
{
	last_sortby = get_sortby();
	expand_buttons.clear();
	expand_button_keys.clear();
	category_count = 0;

	remove_all();
	set_table_layout(1,0);

	fill();

	// Containers ignore the first event after their components changed. Since the list is
	// rebuilt while drawing and not while handling an event, the new components can get events at once.
	// Otherwise clicks get lost often, since the list of a busy stop is rebuilt all the time.
	clear_dirty();
	FOR(vector_tpl<gui_container_t *>, const table, new_tables) {
		table->clear_dirty();
	}
	new_tables.clear();

	set_size(get_min_size());
}


void gui_freight_list_t::add_category(uint8 catg_index, const vector_tpl<ware_t> &wares, uint64 total, uint32 capacity, uint32 overload_capacity)
{
	const freight_list_sorter_t::sort_mode_t sortby = (freight_list_sorter_t::sort_mode_t)get_sortby();
	const char *arrow = get_list_arrow();

	if(  category_count > 0  ) {
		new_component<gui_margin_t>(0, D_V_SPACE);
	}
	category_count++;

	// heading: category, amount / capacity and bar
	const goods_desc_t *info = goods_manager_t::get_info_catg_index(catg_index);
	// overloading (of passenger cars) is allowed up to overload_capacity
	const bool overloaded = total > capacity;
	const bool overcrowded = total > (capacity > overload_capacity ? capacity : overload_capacity);
	add_list_table(5,1)->set_alignment(ALIGN_LEFT | ALIGN_CENTER_V);
	{
		new_component<gui_color_dot_t>(info->get_color(), true);

		new_component<gui_label_t>(info->get_catg()==0 ? info->get_name() : info->get_catg_name());

		char total_str[64], capacity_str[64];
		number_to_string(total_str, (double)total, 0);
		number_to_string(capacity_str, (double)capacity, 0);
		gui_label_buf_t *lb = new_component<gui_label_buf_t>(overcrowded ? SYSCOL_TEXT_STRONG : SYSCOL_TEXT);
		lb->buf().printf("%s / %s", total_str, capacity_str);
		if(  overload_capacity > capacity  ) {
			char overload_str[64];
			number_to_string(overload_str, (double)overload_capacity, 0);
			lb->buf().printf("(%s)", overload_str);
		}
		lb->buf().append(translator::translate(info->get_mass()));
		lb->update();

		if(  capacity > 0  ) {
			const sint32 value = total > 0x7FFFFFFF ? 0x7FFFFFFF : (sint32)total;
			new_component<gui_capacity_bar_t>(value, capacity > 0x7FFFFFFF ? 0x7FFFFFFF : (sint32)capacity, overcrowded ? color_idx_to_rgb(COL_RED) : overloaded ? color_idx_to_rgb(COL_ORANGE) : SYSCOL_LOADINGBAR_PROGRESS);
		}
		else {
			new_component<gui_empty_t>();
		}
		new_component<gui_fill_t>();
	}
	end_table();

	if(  total == 0  ) {
		return;
	}

	vector_tpl<ware_t> sorted;
	freight_list_sorter_t::sort_ware_list(wares, sortby, sorted);

	// entries: amount, destination or next stop, and the operators serving the next stop
	// (right alignment for the amounts; the last column fills the remaining width)
	add_list_table(3,0)->set_alignment(ALIGN_RIGHT | ALIGN_CENTER_V);
	{
		FOR(vector_tpl<ware_t>, const& ware, sorted) {
			halthandle_t const target = ware.get_ziel();
			halthandle_t const via    = ware.get_zwischenziel();
			const bool is_factory_going = sortby != freight_list_sorter_t::by_via_sum  &&  ware.to_factory; // exclude merged packets
			// merged packets of different destinations: only the next stop is shown, the destinations on demand
			const bool is_merged_transfer = sortby == freight_list_sorter_t::by_via_sum  &&  via.is_bound()  &&  via != target;
			const bool is_expanded = is_merged_transfer  &&  expanded.is_contained( expand_key(ware) );

			if(  is_merged_transfer  ) {
				button_t *bt = new_component<button_t>();
				bt->init( is_expanded ? button_t::arrowdown : button_t::arrowright, NULL );
				bt->add_listener(this);
				expand_buttons.append(bt);
				expand_button_keys.append( expand_key(ware) );
			}
			else {
				new_component<gui_margin_t>( max(D_ARROW_RIGHT_WIDTH, D_ARROW_DOWN_WIDTH), 0 );
			}

			char amount_str[64];
			number_to_string(amount_str, (double)ware.menge, 0);
			gui_label_buf_t *lb_amount = new_component<gui_label_buf_t>();
			if(  sortby == freight_list_sorter_t::by_via_sum  &&  ware.is_goods_amount_maxed()  ) {
				lb_amount->buf().append(">=");
			}
			lb_amount->buf().append(amount_str);
			lb_amount->update();

			add_list_table(0,1)->set_alignment(ALIGN_LEFT | ALIGN_CENTER_V);
			{
				char via_text[256] = "";
				if(  via.is_bound()  ) {
					snprintf(via_text, lengthof(via_text), translator::translate("via %s\n"), via->get_name());
					if(  char *nl = strchr(via_text, '\n')  ) {
						*nl = 0;
					}
				}

				gui_label_buf_t *lb_name = new_component<gui_label_buf_t>();
				lb_name->buf().append(arrow);
				if(  is_merged_transfer  ) {
					lb_name->buf().append(via_text);
				}
				else {
					if(  is_factory_going  ) {
						const fabrik_t *const factory = fabrik_t::get_fab(ware.get_zielpos());
						lb_name->buf().printf("%s <%i,%i>", factory ? translator::translate(factory->get_name()) : "Invalid Factory", ware.get_zielpos().x, ware.get_zielpos().y);
					}
					else {
						lb_name->buf().append(target.is_bound() ? target->get_name() : translator::translate("Error in Routing"));
					}
					// transfer: show the next stop
					if(  via.is_bound()  &&  (via != target  ||  is_factory_going)  ) {
						gui_label_buf_t *lb_via = new_component<gui_label_buf_t>(SYSCOL_BUTTON_TEXT_DISABLED);
						lb_via->buf().append(via_text);
						lb_via->update();
					}
				}
				lb_name->update();

				// freight categories may contain several goods
				if(  info->get_catg() != 0  ) {
					gui_label_buf_t *lb_goods = new_component<gui_label_buf_t>(SYSCOL_BUTTON_TEXT_DISABLED);
					lb_goods->buf().printf("(%s)", translator::translate(ware.get_name()));
					lb_goods->update();
				}

				add_player_dots( get_serving_players(via) );

				new_component<gui_fill_t>();
			}
			end_table();

			if(  is_expanded  ) {
				add_final_destinations(wares, ware);
			}
		}
	}
	end_table();
}


void gui_freight_list_t::add_final_destinations(const vector_tpl<ware_t> &wares, const ware_t &entry)
{
	struct destination_t {
		halthandle_t next_transfer; // where to change after the next stop of the entry, if not going there directly
		halthandle_t halt;
		uint64 amount;
	};
	vector_tpl<destination_t> destinations;
	FOR(vector_tpl<ware_t>, const& ware, wares) {
		// the same goods as merged into the entry by freight_list_sorter_t::sort_ware_list()
		if(  ware.get_desc() == goods_manager_t::none  ||  ware.menge == 0  ||  ware.get_index() != entry.get_index()  ||
			ware.get_zwischenziel() != entry.get_zwischenziel()  ||  ware.get_ziel() == ware.get_zwischenziel()  ) {
			continue;
		}
		// transit_halts: the stops to leave the convoy, starting with the next stop and ending with the destination
		const vector_tpl<halthandle_t> &transit = ware.get_transit_halts();
		const halthandle_t next_transfer = transit.get_count() > 2 ? transit[1] : halthandle_t();
		bool found = false;
		for(  uint32 i = 0;  i < destinations.get_count();  i++  ) {
			if(  destinations[i].halt == ware.get_ziel()  &&  destinations[i].next_transfer == next_transfer  ) {
				destinations[i].amount += ware.menge;
				found = true;
				break;
			}
		}
		if(  !found  ) {
			destination_t d = { next_transfer, ware.get_ziel(), ware.menge };
			destinations.append(d);
		}
	}
	if(  destinations.empty()  ) {
		return;
	}
	std::sort( destinations.begin(), destinations.end(), [](const destination_t &a, const destination_t &b) {
		if(  a.amount != b.amount  ) {
			return a.amount > b.amount;
		}
		if(  a.halt != b.halt  ) {
			if(  a.halt.is_bound()  &&  b.halt.is_bound()  ) {
				return natural_strcmp(a.halt->get_name(), b.halt->get_name()) < 0;
			}
			return b.halt.is_bound(); // unknown destinations last
		}
		// same destination over different transfers: direct ones first
		if(  a.next_transfer.is_bound()  &&  b.next_transfer.is_bound()  ) {
			return natural_strcmp(a.next_transfer->get_name(), b.next_transfer->get_name()) < 0;
		}
		return b.next_transfer.is_bound();
	} );

	// below the name of the entry, connected to it by tree lines
	new_component<gui_empty_t>();
	new_component<gui_empty_t>();
	gui_aligned_container_t *tree = add_list_table(3,0);
	tree->set_alignment(ALIGN_RIGHT | ALIGN_CENTER_V);
	tree->set_spacing( scr_size(D_H_SPACE, 0) ); // no gaps between the tree lines
	{
		for(  uint32 i = 0;  i < destinations.get_count();  i++  ) {
			const destination_t &d = destinations[i];

			new_component<gui_tree_branch_t>( i == 0, i+1 == destinations.get_count() );

			char amount_str[64];
			number_to_string(amount_str, (double)d.amount, 0);
			gui_label_buf_t *lb_amount = new_component<gui_label_buf_t>(SYSCOL_BUTTON_TEXT_DISABLED);
			lb_amount->buf().append(amount_str);
			lb_amount->update();

			add_list_table(0,1)->set_alignment(ALIGN_LEFT | ALIGN_CENTER_V);
			{
				// further transfer: next transfer -> destination
				if(  d.next_transfer.is_bound()  ) {
					gui_label_buf_t *lb_transfer = new_component<gui_label_buf_t>(SYSCOL_BUTTON_TEXT_DISABLED);
					lb_transfer->buf().append( d.next_transfer->get_name() );
					lb_transfer->update();
				}
				gui_label_buf_t *lb_name = new_component<gui_label_buf_t>();
				if(  d.next_transfer.is_bound()  ) {
					lb_name->buf().append( get_list_arrow() );
				}
				lb_name->buf().append( d.halt.is_bound() ? d.halt->get_name() : translator::translate("Error in Routing") );
				lb_name->update();
				new_component<gui_fill_t>();
			}
			end_table();
		}
	}
	end_table();
}
