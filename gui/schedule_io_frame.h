/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef GUI_SCHEDULE_IO_FRAME_H
#define GUI_SCHEDULE_IO_FRAME_H


#include "savegame_frame.h"

#include "components/gui_button.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"

#include "../utils/cbuffer_t.h"


class player_t;
class schedule_t;
class schedule_gui_t;


/**
 * File dialog to export the schedule being edited, or to import one into it.
 * The clipboard is offered as an alternative to a file from within the same dialog.
 */
class schedule_io_frame_t : public savegame_frame_t
{
private:
	bool do_export;

	/// only used while importing; the schedule window the import is applied to
	schedule_gui_t *owner;

	/// export only: the text is built once at construction, so the dialog does not
	/// depend on the schedule window still being alive
	cbuffer_t export_buf;

	button_t bt_clipboard;
	gui_label_t lb_mode;
	gui_combobox_t mode_combo;

	/// validates and applies the text and shows the outcome; true when the schedule was applied
	bool handle_import_text(const char *text);

protected:
	bool item_action(const char *fullpath) OVERRIDE;
	bool ok_action  (const char *fullpath) OVERRIDE;
	const char *get_info(const char *fname) OVERRIDE;

public:
	schedule_io_frame_t(schedule_gui_t *owner, bool do_export, const schedule_t *schedule, player_t *player, const char *source_name);

	bool action_triggered(gui_action_creator_t *, value_t) OVERRIDE;

	const char *get_help_filename() const OVERRIDE { return "schedule_io.txt"; }
};

#endif
