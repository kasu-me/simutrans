/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_SCHEDULE_IO_H
#define DATAOBJ_SCHEDULE_IO_H


#include "../simtypes.h"


class cbuffer_t;
class player_t;
class schedule_t;


/**
 * How an imported schedule is merged into the schedule being edited.
 */
enum schedule_import_mode_t {
	SCHEDULE_IMPORT_REPLACE_ALL    = 0, ///< replace stops and all settings
	SCHEDULE_IMPORT_DEPARTURE_ONLY = 1  ///< keep the stops, only overwrite departure time settings
};

/// version of the text format written by schedule_export_text()
#define SCHEDULE_IO_FORMAT_VERSION 1


/**
 * Writes @p schedule as human readable text, suitable for a file or the clipboard.
 * @p source_name is written as an informational "source" line, may be NULL.
 */
void schedule_export_text( cbuffer_t &buf, const schedule_t *schedule, const player_t *player, const char *source_name );

/**
 * Parses @p text and applies it to @p target.
 * Everything is validated before anything is written, so on failure @p target is left untouched.
 * @param errmsg  receives a translated reason when the import is rejected
 * @param warnmsg receives translated notes about things that could not be restored exactly
 * @return true if the schedule was applied
 */
bool schedule_import_text( const char *text, schedule_t *target, schedule_import_mode_t mode, const player_t *player, cbuffer_t &errmsg, cbuffer_t &warnmsg );

/*
 * spacing_shift and delay_tolerance are stored as fractions of a month (0..spacing_shift_divisor),
 * which map linearly onto a virtual 24h day (86400 "seconds").
 */
void   schedule_linear_raw_to_hms( uint16 raw, uint16 divisor, uint8 &h, uint8 &m, uint8 &s );
uint16 schedule_linear_hms_to_raw( uint8 h, uint8 m, uint8 s, uint16 divisor );

#endif
