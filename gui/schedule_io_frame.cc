/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <string.h>

#include "schedule_io_frame.h"

#include "messagebox.h"
#include "schedule_gui.h"
#include "simwin.h"

#include "../pathes.h"
#include "../simdebug.h"
#include "../dataobj/environment.h"
#include "../dataobj/schedule.h"
#include "../dataobj/schedule_io.h"
#include "../dataobj/translator.h"
#include "../sys/simsys.h"
#include "../unicode.h"


/// generous upper bound for an imported schedule; a 254 stop schedule is well below this
#define SCHEDULE_IO_MAX_TEXT (256*1024)


/**
 * Turns a line or convoy name into something usable as a file name, so it can seed the
 * input field. Characters that cannot appear in a file name become '_'.
 * Copies whole UTF-8 characters only: cutting a name in the middle of a multi-byte
 * character would leave a broken glyph in the file name.
 */
static void sanitize_filename(const char *src, char *dest, size_t dest_size)
{
	size_t j = 0;
	for(  size_t i = 0;  src  &&  src[i];  ) {
		// length of this (possibly multi-byte) character
		size_t char_len = 1;
		while(  is_cont_char((utf8)src[i+char_len])  ) {
			char_len++;
		}
		if(  j + char_len + 1 > dest_size  ) {
			// does not fit as a whole, rather stop than split it
			break;
		}
		const unsigned char c = (unsigned char)src[i];
		if(  char_len == 1  &&  (c < 0x20  ||  strchr("\\/:*?\"<>|", (char)c) != NULL)  ) {
			dest[j++] = '_';
		}
		else {
			for(  size_t k = 0;  k < char_len;  k++  ) {
				dest[j++] = src[i+k];
			}
		}
		i += char_len;
	}
	dest[j] = 0;
}


schedule_io_frame_t::schedule_io_frame_t(schedule_gui_t *owner_, bool do_export_, const schedule_t *schedule, player_t *player, const char *source_name) :
	savegame_frame_t(".sch", false, SCHEDULE_PATH_X, env_t::show_delete_buttons)
{
	do_export = do_export_;
	owner     = owner_;

	if(  do_export  ) {
		set_name(translator::translate("Export schedule"));
		if(  schedule != NULL  ) {
			schedule_export_text(export_buf, schedule, player, source_name);
		}

		// set_filename() expects a name that still carries its extension and drops the last
		// four bytes, so hand it the name with the suffix rather than the bare name.
		// It also ignores anything shorter than 8 bytes, hence the length check.
		char fname[128];
		sanitize_filename(source_name, fname, lengthof(fname) - 4);
		if(  fname[0]  ) {
			strcat(fname, ".sch");
			if(  strlen(fname) >= 8  ) {
				set_filename(fname);
			}
		}

		bt_clipboard.init(button_t::roundbox, "Copy to clipboard");
		bt_clipboard.set_tooltip("Copy the schedule to the system clipboard instead of a file");
		bt_clipboard.add_listener(this);
		bottom_left_frame.add_component(&bt_clipboard);
	}
	else {
		set_name(translator::translate("Import schedule"));

		gui_aligned_container_t *table = bottom_left_frame.add_table(2, 1);
			lb_mode.set_text("Import mode:");
			table->add_component(&lb_mode);

			mode_combo.set_unsorted();
			mode_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Replace whole schedule"), SYSCOL_TEXT );
			mode_combo.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate("Apply departure time settings only"), SYSCOL_TEXT );
			mode_combo.set_selection(SCHEDULE_IMPORT_REPLACE_ALL);
			table->add_component(&mode_combo);
		bottom_left_frame.end_table();

		bt_clipboard.init(button_t::roundbox, "Paste from clipboard");
		bt_clipboard.set_tooltip("Read the schedule from the system clipboard instead of a file");
		bt_clipboard.add_listener(this);
		bottom_left_frame.add_component(&bt_clipboard);
	}

	set_focus(NULL);
}


const char *schedule_io_frame_t::get_info(const char *)
{
	return "";
}


bool schedule_io_frame_t::item_action(const char *fullpath)
{
	if(  do_export  ) {
		return ok_action(fullpath);
	}

	FILE *f = dr_fopen(fullpath, "rb");
	if(  f == NULL  ) {
		create_win( new news_img(translator::translate("Could not read the schedule file.\n")), w_info, magic_none );
		return false;
	}
	char *text = (char *)malloc(SCHEDULE_IO_MAX_TEXT);
	if(  text == NULL  ) {
		fclose(f);
		return false;
	}
	const size_t len = fread(text, 1, SCHEDULE_IO_MAX_TEXT-1, f);
	fclose(f);
	text[len] = 0;

	const bool applied = handle_import_text(text);
	free(text);
	// on failure the dialog stays open so another file can be picked
	return applied;
}


bool schedule_io_frame_t::ok_action(const char *fullpath)
{
	if(  !do_export  ) {
		// typing a name makes no sense when importing
		return true;
	}

	FILE *f = dr_fopen(fullpath, "wb");
	if(  f == NULL  ) {
		create_win( new news_img(translator::translate("Could not write the schedule file.\n")), w_info, magic_none );
		return false;
	}
	if(  export_buf.len() > 0  ) {
		fwrite((const char *)export_buf, 1, export_buf.len(), f);
	}
	fclose(f);
	create_win( new news_img(translator::translate("Schedule was exported.\n")), w_time_delete, magic_none );
	return true;
}


bool schedule_io_frame_t::handle_import_text(const char *text)
{
	if(  !win_is_open(owner)  ) {
		create_win( new news_img(translator::translate("The schedule window was closed.\n")), w_info, magic_none );
		return false;
	}

	const schedule_import_mode_t mode = mode_combo.get_selection() == SCHEDULE_IMPORT_DEPARTURE_ONLY
		? SCHEDULE_IMPORT_DEPARTURE_ONLY : SCHEDULE_IMPORT_REPLACE_ALL;

	cbuffer_t errmsg, warnmsg;
	if(  !owner->apply_imported_schedule(text, mode, errmsg, warnmsg)  ) {
		create_win( new news_img(errmsg), w_info, magic_none );
		return false;
	}

	cbuffer_t done;
	done.append(translator::translate("Schedule was imported.\n"));
	if(  warnmsg.len() > 0  ) {
		done.append(warnmsg);
		create_win( new news_img(done), w_info, magic_none );
	}
	else {
		create_win( new news_img(done), w_time_delete, magic_none );
	}
	return true;
}


bool schedule_io_frame_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	if(  comp == &bt_clipboard  ) {
		if(  do_export  ) {
			if(  export_buf.len() > 0  ) {
				dr_copy( export_buf, export_buf.len() );
				create_win( new news_img(translator::translate("Schedule was copied to clipboard.\n")), w_time_delete, magic_none );
			}
			destroy_win(this);
		}
		else {
			char *text = (char *)calloc(SCHEDULE_IO_MAX_TEXT, 1);
			if(  text != NULL  ) {
				// dr_paste() inserts at the given position, so an empty buffer just receives the text
				dr_paste(text, SCHEDULE_IO_MAX_TEXT-1);
				const bool applied = handle_import_text(text);
				free(text);
				if(  applied  ) {
					destroy_win(this);
				}
			}
		}
		return true;
	}
	return savegame_frame_t::action_triggered(comp, v);
}
