/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_COMPONENTS_GUI_CONVOIINFO_H
#define GUI_COMPONENTS_GUI_CONVOIINFO_H


#include "gui_aligned_container.h"
#include "gui_label.h"
#include "gui_scrolled_list.h"
#include "gui_speedbar.h"
#include "../../convoihandle_t.h"
#include "../../linehandle_t.h"

/**
 * Convoi info stats, like loading status bar
 * One element of the vehicle list display
 */
class gui_convoiinfo_t : public gui_aligned_container_t, public gui_scrolled_list_t::scrollitem_t
{
private:
	/**
	* Handle Convois to be displayed.
	* If chain_line is bound, this is the head convoy of the displayed train.
	*/
	convoihandle_t cnv;

	/**
	* When bound, this element stands for the convoys that are coupled to cnv and that
	* belong to this line: they are displayed as one single train. The chain is followed
	* from cnv until a convoy of another line or the end of the chain is reached.
	*/
	linehandle_t chain_line;

	/// aggregated loading of the displayed coupled train, only used when chain_line is bound
	sint32 chain_loading_level, chain_loading_limit;

	/// 100 while the displayed convoy(s) are overloaded, 0 otherwise. @see update_label()
	sint32 loading_overload;

	gui_speedbar_t filled_bar;
	gui_label_buf_t label_name, label_line, label_profit, label_next_halt;
	button_t pos_next_halt;
	gui_aligned_container_t *container_next_halt;

	/**
	* Opens the info window of each convoy displayed by this element. The windows of a
	* coupled train are cascaded so that they do not cover each other.
	*/
	void open_info_windows() const;

public:
	/**
	* @param cnv the handle for the Convoi to be displayed.
	* @param chain_line when bound, cnv and the convoys coupled behind it that belong to
	*        this line are displayed together as one entry.
	*/
	gui_convoiinfo_t(convoihandle_t cnv, linehandle_t chain_line = linehandle_t());

	bool infowin_event(event_t const*) OVERRIDE;

	/**
	* Draw the component
	*/
	void draw(scr_coord offset) OVERRIDE;

	void update_label();

	const char* get_text() const OVERRIDE;

	bool is_valid() const OVERRIDE { return cnv.is_bound(); }

	convoihandle_t get_cnv() const { return cnv; }

	using gui_aligned_container_t::get_min_size;
	using gui_aligned_container_t::get_max_size;
};

#endif
