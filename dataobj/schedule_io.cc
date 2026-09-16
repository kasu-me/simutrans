/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <string.h>
#include <string>

#include "schedule_io.h"

#include "schedule.h"
#include "schedule_entry.h"
#include "translator.h"

#include "../simworld.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../simdepot.h"
#include "../simversion.h"
#include "../boden/grund.h"
#include "../player/simplay.h"
#include "../tpl/vector_tpl.h"
#include "../utils/cbuffer_t.h"


static karte_ptr_t welt;


/* ----------------------------------------------------------------------------
 * name tables
 *
 * Flags are written as a list of names instead of a bit mask, so an exported
 * schedule can be read and edited by hand. Never rename an existing entry: the
 * name is the file format. New bits are appended and bump SCHEDULE_IO_FORMAT_VERSION.
 * ---------------------------------------------------------------------------- */

struct flag_name_t {
	const char *name;
	uint32 bit;
};

static const flag_name_t schedule_flag_names[] = {
	{ "temporary",              schedule_t::TEMPORARY },
	{ "same_dep_time",          schedule_t::SAME_DEP_TIME },
	{ "full_load_acceleration", schedule_t::FULL_LOAD_ACCELERATION },
	{ "full_load_time",         schedule_t::FULL_LOAD_TIME },
	{ "reverse_default",        schedule_t::REVERSE_DEFAULT },
	{ "no_use_electric",        schedule_t::NO_USE_ELECTRIC },
	{ NULL, 0 }
};

static const flag_name_t stop_flag_names[] = {
	{ "wait_for_coupling",           schedule_entry_t::WAIT_FOR_COUPLING },
	{ "try_coupling",                schedule_entry_t::TRY_COUPLING },
	{ "no_load",                     schedule_entry_t::NO_LOAD },
	{ "no_unload",                   schedule_entry_t::NO_UNLOAD },
	{ "wait_for_time",               schedule_entry_t::WAIT_FOR_TIME },
	{ "unload_all",                  schedule_entry_t::UNLOAD_ALL },
	{ "load_before_dep",             schedule_entry_t::LOAD_BEFORE_DEP },
	{ "transfer_interval",           schedule_entry_t::TRANSFER_INTERVAL },
	{ "reverse_convoy",              schedule_entry_t::REVERSE_CONVOY },
	{ "reverse_coupling",            schedule_entry_t::REVERSE_COUPLING },
	{ "wait_coupling_done",          schedule_entry_t::WAIT_COUPLING_DONE },
	{ "max_speed_kmh_of_convoi",     schedule_entry_t::MAX_SPEED_KMH_OF_CONVOI },
	{ "no_overtake",                 schedule_entry_t::NO_OVERTAKE },
	{ "uncouple_child",              schedule_entry_t::UNCOUPLE_CHILD },
	{ "pass_stop",                   schedule_entry_t::PASS_STOP },
	{ "no_go_no_users",              schedule_entry_t::NO_GO_NO_USERS },
	{ "temp_load",                   schedule_entry_t::TEMP_LOAD },
	{ "temp_unload",                 schedule_entry_t::TEMP_UNLOAD },
	{ "temp_unload_all",             schedule_entry_t::TEMP_UNLOAD_ALL },
	{ "balance_speed_kmh_of_convoi", schedule_entry_t::BALANCE_SPEED_KMH_OF_CONVOI },
	{ NULL, 0 }
};

struct schedule_type_name_t {
	const char *name;
	schedule_t::schedule_type type;
};

static const schedule_type_name_t schedule_type_names[] = {
	{ "truck",       schedule_t::truck_schedule },
	{ "train",       schedule_t::train_schedule },
	{ "ship",        schedule_t::ship_schedule },
	{ "air",         schedule_t::airplane_schedule },
	{ "monorail",    schedule_t::monorail_schedule },
	{ "tram",        schedule_t::tram_schedule },
	{ "maglev",      schedule_t::maglev_schedule },
	{ "narrowgauge", schedule_t::narrowgauge_schedule },
	{ NULL, schedule_t::schedule }
};


static const char *schedule_type_to_name(schedule_t::schedule_type t)
{
	for(  const schedule_type_name_t *n = schedule_type_names;  n->name;  n++  ) {
		if(  n->type == t  ) {
			return n->name;
		}
	}
	return "unknown";
}


/* ----------------------------------------------------------------------------
 * time conversion (shared with the schedule dialog)
 * ---------------------------------------------------------------------------- */

void schedule_linear_raw_to_hms(uint16 raw, uint16 divisor, uint8 &h, uint8 &m, uint8 &s)
{
	uint32 seconds = divisor>0 ? (uint32)( ((uint64)raw * 86400ull) / divisor ) : 0;
	if(  seconds > 86400  ) {
		seconds = 86400;
	}
	h = (uint8)(seconds / 3600);
	m = (uint8)((seconds % 3600) / 60);
	s = (uint8)(seconds % 60);
}


uint16 schedule_linear_hms_to_raw(uint8 h, uint8 m, uint8 s, uint16 divisor)
{
	const uint32 seconds = (uint32)h*3600u + (uint32)m*60u + (uint32)s;
	// round to nearest, since a divisor step may not evenly divide a second
	const uint64 raw = ( (uint64)seconds * divisor + 43200ull ) / 86400ull;
	return raw > 0xFFFFull ? (uint16)0xFFFF : (uint16)raw;
}


/* ----------------------------------------------------------------------------
 * export
 * ---------------------------------------------------------------------------- */

/// appends "  # h:mm:ss\n" for a value given in spacing_shift_divisor units of a month
static void append_hms_comment(cbuffer_t &buf, uint16 raw, uint16 divisor)
{
	uint8 h, m, s;
	schedule_linear_raw_to_hms(raw, divisor, h, m, s);
	buf.printf("  # %u:%02u:%02u\n", (uint32)h, (uint32)m, (uint32)s);
}


static void append_flag_list(cbuffer_t &buf, uint32 flags, const flag_name_t *table)
{
	bool first = true;
	for(  const flag_name_t *f = table;  f->name;  f++  ) {
		if(  flags & f->bit  ) {
			if(  !first  ) {
				buf.append(", ");
			}
			buf.append(f->name);
			first = false;
		}
	}
}


/// name of the line the handle points at, or "" when unbound
static const char *line_name_of(linehandle_t line)
{
	return line.is_bound() ? line->get_name() : "";
}


void schedule_export_text( cbuffer_t &buf, const schedule_t *schedule, const player_t *player, const char *source_name )
{
	const uint16 divisor = welt->get_settings().get_spacing_shift_divisor();
	const waytype_t wt = schedule->get_waytype();

	buf.append("# Simutrans OTRP schedule\n");
	buf.append("# Lines starting with '#' are comments. Stop names are informational only.\n");
	buf.append("format = otrp-schedule\n");
	buf.printf("format_version = %d\n", SCHEDULE_IO_FORMAT_VERSION);
	buf.printf("otrp_version = %d\n", OTRP_VERSION_MAJOR);
	buf.printf("schedule_type = %s\n", schedule_type_to_name(schedule->get_type()));
	buf.printf("source = %s\n", source_name ? source_name : "");
	buf.printf("spacing_shift_divisor = %u\n", (uint32)divisor);
	buf.append("schedule_flags = ");
	append_flag_list(buf, schedule->get_flags(), schedule_flag_names);
	buf.append("\n");
	buf.printf("max_speed = %u\n", (uint32)schedule->get_max_speed());
	buf.printf("additional_base_waiting_time = %u\n", schedule->get_additional_base_waiting_time());
	buf.printf("departure_slot_group = %s\n", line_name_of(schedule->get_departure_slot_group_id()));
	buf.printf("next_line = %s\n", line_name_of(schedule->get_next_line()));
	buf.printf("entry_count = %u\n", (uint32)schedule->get_count());

	for(  uint8 i = 0;  i < schedule->get_count();  i++  ) {
		const schedule_entry_t &e = schedule->at(i);
		buf.append("\n[entry]\n");
		buf.printf("# %s\n", schedule_t::get_stop_name(welt, player, e, wt));
		buf.printf("pos = %i, %i, %i\n", (int)e.pos.x, (int)e.pos.y, (int)e.pos.z);
		buf.append("stop_flags = ");
		append_flag_list(buf, e.get_stop_flags(), stop_flag_names);
		buf.append("\n");
		buf.printf("minimum_loading = %u\n", (uint32)e.minimum_loading);
		buf.printf("maximum_loading = %u\n", (uint32)e.maximum_loading);
		buf.printf("waiting_time_shift = %u\n", (uint32)e.waiting_time_shift);
		buf.printf("spacing = %u", (uint32)e.spacing);
		if(  e.spacing > 0  ) {
			// the resulting headway, for readability
			const uint32 seconds = 86400u / e.spacing;
			buf.printf("  # every %u:%02u:%02u\n", seconds/3600u, (seconds%3600u)/60u, seconds%60u);
		}
		else {
			buf.append("\n");
		}
		buf.printf("spacing_shift = %u", (uint32)e.spacing_shift);
		append_hms_comment(buf, e.spacing_shift, divisor);
		buf.printf("delay_tolerance = %u", (uint32)e.delay_tolerance);
		append_hms_comment(buf, e.delay_tolerance, divisor);
		buf.printf("max_speed_kmh_of_convoi = %u\n", (uint32)e.max_speed_kmh_of_convoi);
		buf.printf("balance_speed_kmh_of_convoi = %u\n", (uint32)e.balance_speed_kmh_of_convoi);
		buf.printf("length_coupling_done = %u\n", (uint32)e.length_coupling_done);
	}
}


/* ----------------------------------------------------------------------------
 * import
 * ---------------------------------------------------------------------------- */

/// everything read from one [entry] block
struct parsed_entry_t {
	koord3d pos;
	bool   has_pos;
	uint32 stop_flags;
	uint32 minimum_loading;
	uint32 maximum_loading;
	uint32 waiting_time_shift;
	uint32 spacing;
	uint32 spacing_shift;
	uint32 delay_tolerance;
	uint32 max_speed_kmh_of_convoi;
	uint32 balance_speed_kmh_of_convoi;
	uint32 length_coupling_done;

	parsed_entry_t() :
		pos(koord3d::invalid), has_pos(false), stop_flags(0), minimum_loading(0), maximum_loading(100),
		waiting_time_shift(0), spacing(1), spacing_shift(0), delay_tolerance(0),
		max_speed_kmh_of_convoi(0), balance_speed_kmh_of_convoi(0), length_coupling_done(0) {}
};

/// everything read from the header
struct parsed_header_t {
	bool   has_format;
	uint32 format_version;
	bool   has_type;
	schedule_t::schedule_type type;
	bool   has_divisor;
	uint32 spacing_shift_divisor;
	uint32 schedule_flags;
	uint32 max_speed;
	uint32 additional_base_waiting_time;
	bool   has_entry_count;
	uint32 entry_count;
	std::string departure_slot_group;
	std::string next_line;

	parsed_header_t() :
		has_format(false), format_version(0), has_type(false), type(schedule_t::schedule),
		has_divisor(false), spacing_shift_divisor(0), schedule_flags(0), max_speed(0),
		additional_base_waiting_time(0), has_entry_count(false), entry_count(0) {}
};


static void trim(std::string &s)
{
	size_t b = 0;
	while(  b < s.size()  &&  (s[b]==' ' || s[b]=='\t' || s[b]=='\r' || s[b]=='\n')  ) {
		b++;
	}
	size_t e = s.size();
	while(  e > b  &&  (s[e-1]==' ' || s[e-1]=='\t' || s[e-1]=='\r' || s[e-1]=='\n')  ) {
		e--;
	}
	s = s.substr(b, e-b);
}


/// strict unsigned parse: the whole token must be digits
static bool parse_uint(const std::string &s, uint32 &out)
{
	if(  s.empty()  ) {
		return false;
	}
	uint64 v = 0;
	for(  size_t i = 0;  i < s.size();  i++  ) {
		if(  s[i] < '0'  ||  s[i] > '9'  ) {
			return false;
		}
		v = v*10 + (uint32)(s[i]-'0');
		if(  v > 0xFFFFFFFFull  ) {
			return false;
		}
	}
	out = (uint32)v;
	return true;
}


/// a plain number, or "h:mm:ss" which is converted using @p divisor (for hand edited files)
static bool parse_time_or_uint(const std::string &s, uint16 divisor, uint32 &out)
{
	if(  s.find(':') == std::string::npos  ) {
		return parse_uint(s, out);
	}
	uint32 part[3] = { 0, 0, 0 };
	size_t start = 0;
	for(  int i = 0;  i < 3;  i++  ) {
		const size_t sep = s.find(':', start);
		if(  i < 2  &&  sep == std::string::npos  ) {
			return false; // needs h:mm:ss
		}
		std::string tok = (i<2) ? s.substr(start, sep-start) : s.substr(start);
		trim(tok);
		if(  !parse_uint(tok, part[i])  ) {
			return false;
		}
		start = (sep==std::string::npos) ? s.size() : sep+1;
	}
	if(  part[0] > 24  ||  part[1] > 59  ||  part[2] > 59  ) {
		return false;
	}
	out = schedule_linear_hms_to_raw((uint8)part[0], (uint8)part[1], (uint8)part[2], divisor);
	return true;
}


static bool parse_pos(const std::string &s, koord3d &out)
{
	sint32 v[3];
	size_t start = 0;
	for(  int i = 0;  i < 3;  i++  ) {
		const size_t sep = s.find(',', start);
		if(  (i<2) == (sep==std::string::npos)  ) {
			return false; // need exactly two commas
		}
		std::string tok = (i<2) ? s.substr(start, sep-start) : s.substr(start);
		trim(tok);
		bool negative = false;
		if(  !tok.empty()  &&  tok[0]=='-'  ) {
			negative = true;
			tok = tok.substr(1);
		}
		uint32 raw;
		if(  !parse_uint(tok, raw)  ||  raw > 32767  ) {
			return false;
		}
		v[i] = negative ? -(sint32)raw : (sint32)raw;
		start = (sep==std::string::npos) ? s.size() : sep+1;
	}
	out = koord3d((sint16)v[0], (sint16)v[1], (sint8)v[2]);
	return true;
}


/// comma separated list of flag names; empty or "none" means no bit set
static bool parse_flag_list(const std::string &s, const flag_name_t *table, uint32 &out, std::string &bad_name)
{
	out = 0;
	size_t start = 0;
	while(  true  ) {
		const size_t sep = s.find(',', start);
		std::string tok = s.substr(start, sep==std::string::npos ? std::string::npos : sep-start);
		trim(tok);
		if(  !tok.empty()  &&  tok != "none"  ) {
			const flag_name_t *f = table;
			for(  ;  f->name;  f++  ) {
				if(  tok == f->name  ) {
					out |= f->bit;
					break;
				}
			}
			if(  f->name == NULL  ) {
				bad_name = tok;
				return false;
			}
		}
		if(  sep == std::string::npos  ) {
			return true;
		}
		start = sep+1;
	}
}


/// finds a line of the given player by name; ambiguous names resolve to the first match
static linehandle_t find_line_by_name(const player_t *player, const std::string &name, bool &ambiguous)
{
	ambiguous = false;
	linehandle_t found;
	if(  player == NULL  ||  name.empty()  ) {
		return found;
	}
	FOR(  vector_tpl<linehandle_t>, const line,  player->simlinemgmt.get_line_list()  ) {
		if(  line.is_bound()  &&  name == line->get_name()  ) {
			if(  found.is_bound()  ) {
				ambiguous = true;
				break;
			}
			found = line;
		}
	}
	return found;
}


static void reject(cbuffer_t &errmsg, const char *reason)
{
	errmsg.append(translator::translate(reason));
}


bool schedule_import_text( const char *text, schedule_t *target, schedule_import_mode_t mode, const player_t *player, cbuffer_t &errmsg, cbuffer_t &warnmsg )
{
	if(  text == NULL  ||  *text == 0  ) {
		reject(errmsg, "Not a schedule file.\n");
		return false;
	}

	const uint16 world_divisor = welt->get_settings().get_spacing_shift_divisor();

	parsed_header_t header;
	vector_tpl<parsed_entry_t> parsed;
	bool in_entry = false;

	// -- parse ---------------------------------------------------------------
	const char *p = text;
	int lineno = 0;
	while(  *p  ) {
		const char *eol = strchr(p, '\n');
		std::string line = eol ? std::string(p, eol-p) : std::string(p);
		p = eol ? eol+1 : p + strlen(p);
		lineno++;

		const size_t hash = line.find('#');
		if(  hash != std::string::npos  ) {
			line = line.substr(0, hash);
		}
		trim(line);
		if(  line.empty()  ) {
			continue;
		}

		if(  line == "[entry]"  ) {
			if(  parsed.get_count() >= 254  ) {
				reject(errmsg, "Too many stops in schedule file.\n");
				return false;
			}
			parsed.append(parsed_entry_t());
			in_entry = true;
			continue;
		}

		const size_t eq = line.find('=');
		if(  eq == std::string::npos  ) {
			reject(errmsg, "Unknown keyword in schedule file.\n");
			errmsg.printf("line %d: %s\n", lineno, line.c_str());
			return false;
		}
		std::string key = line.substr(0, eq);
		std::string val = line.substr(eq+1);
		trim(key);
		trim(val);

		bool key_known = true;
		bool value_ok  = true;
		std::string bad_flag;

		if(  in_entry  ) {
			parsed_entry_t &e = parsed[parsed.get_count()-1];
			if(       key == "pos"                         ) { value_ok = parse_pos(val, e.pos);  e.has_pos = value_ok; }
			else if(  key == "stop_flags"                  ) { value_ok = parse_flag_list(val, stop_flag_names, e.stop_flags, bad_flag); }
			else if(  key == "minimum_loading"             ) { value_ok = parse_uint(val, e.minimum_loading); }
			else if(  key == "maximum_loading"             ) { value_ok = parse_uint(val, e.maximum_loading); }
			else if(  key == "waiting_time_shift"          ) { value_ok = parse_uint(val, e.waiting_time_shift); }
			else if(  key == "spacing"                     ) { value_ok = parse_uint(val, e.spacing); }
			else if(  key == "spacing_shift"               ) { value_ok = parse_time_or_uint(val, world_divisor, e.spacing_shift); }
			else if(  key == "delay_tolerance"             ) { value_ok = parse_time_or_uint(val, world_divisor, e.delay_tolerance); }
			else if(  key == "max_speed_kmh_of_convoi"     ) { value_ok = parse_uint(val, e.max_speed_kmh_of_convoi); }
			else if(  key == "balance_speed_kmh_of_convoi" ) { value_ok = parse_uint(val, e.balance_speed_kmh_of_convoi); }
			else if(  key == "length_coupling_done"        ) { value_ok = parse_uint(val, e.length_coupling_done); }
			else { key_known = false; }
		}
		else {
			uint32 ignored = 0;
			if(  key == "format"  ) {
				if(  val != "otrp-schedule"  ) {
					reject(errmsg, "Not a schedule file.\n");
					return false;
				}
				header.has_format = true;
			}
			else if(  key == "format_version"  ) { value_ok = parse_uint(val, header.format_version); }
			else if(  key == "otrp_version"    ) { value_ok = parse_uint(val, ignored); }
			else if(  key == "source"          ) { /* informational only */ }
			else if(  key == "schedule_type"   ) {
				for(  const schedule_type_name_t *n = schedule_type_names;  n->name;  n++  ) {
					if(  val == n->name  ) {
						header.type = n->type;
						header.has_type = true;
						break;
					}
				}
				value_ok = header.has_type;
			}
			else if(  key == "spacing_shift_divisor" ) {
				value_ok = parse_uint(val, header.spacing_shift_divisor);
				header.has_divisor = value_ok;
			}
			else if(  key == "schedule_flags" ) { value_ok = parse_flag_list(val, schedule_flag_names, header.schedule_flags, bad_flag); }
			else if(  key == "max_speed"      ) { value_ok = parse_uint(val, header.max_speed); }
			else if(  key == "additional_base_waiting_time" ) { value_ok = parse_uint(val, header.additional_base_waiting_time); }
			else if(  key == "departure_slot_group" ) { header.departure_slot_group = val; }
			else if(  key == "next_line"            ) { header.next_line = val; }
			else if(  key == "entry_count"          ) {
				value_ok = parse_uint(val, header.entry_count);
				header.has_entry_count = value_ok;
			}
			else { key_known = false; }
		}

		if(  !key_known  ) {
			reject(errmsg, "Unknown keyword in schedule file.\n");
			errmsg.printf("line %d: %s\n", lineno, key.c_str());
			return false;
		}
		if(  !bad_flag.empty()  ) {
			reject(errmsg, "Unknown keyword in schedule file.\n");
			errmsg.printf("line %d: %s = %s\n", lineno, key.c_str(), bad_flag.c_str());
			return false;
		}
		if(  !value_ok  ) {
			reject(errmsg, "Bad value in schedule file.\n");
			errmsg.printf("line %d: %s = %s\n", lineno, key.c_str(), val.c_str());
			return false;
		}
	}

	// -- validate ------------------------------------------------------------
	if(  !header.has_format  ) {
		reject(errmsg, "Not a schedule file.\n");
		return false;
	}
	if(  header.format_version > SCHEDULE_IO_FORMAT_VERSION  ) {
		reject(errmsg, "This file was made by a newer version.\n");
		return false;
	}
	if(  !header.has_type  ||  header.type != target->get_type()  ) {
		reject(errmsg, "Schedule type does not match.\n");
		errmsg.printf("%s -> %s\n",
			header.has_type ? schedule_type_to_name(header.type) : "?",
			schedule_type_to_name(target->get_type()));
		return false;
	}
	if(  !header.has_divisor  ||  header.spacing_shift_divisor != world_divisor  ) {
		reject(errmsg, "spacing_shift_divisor does not match.\n");
		errmsg.printf("%u -> %u\n", header.spacing_shift_divisor, (uint32)world_divisor);
		return false;
	}
	if(  !header.has_entry_count  ||  header.entry_count != parsed.get_count()  ) {
		reject(errmsg, "Number of stops does not match entry_count.\n");
		errmsg.printf("%u != %u\n", header.entry_count, parsed.get_count());
		return false;
	}
	if(  parsed.empty()  ) {
		reject(errmsg, "Schedule file has no stop.\n");
		return false;
	}
	if(  header.max_speed > 0xFFFF  ) {
		reject(errmsg, "Bad value in schedule file.\n");
		errmsg.append("max_speed\n");
		return false;
	}

	for(  uint32 i = 0;  i < parsed.get_count();  i++  ) {
		const parsed_entry_t &e = parsed[i];
		const char *bad_field = NULL;
		if(       !e.has_pos                                   ) { bad_field = "pos"; }
		else if(  e.minimum_loading > 100                      ) { bad_field = "minimum_loading"; }
		else if(  e.maximum_loading > 100                      ) { bad_field = "maximum_loading"; }
		else if(  e.waiting_time_shift > 0xFFFF                ) { bad_field = "waiting_time_shift"; }
		else if(  e.spacing < 1  ||  e.spacing > world_divisor ) { bad_field = "spacing"; }
		else if(  e.spacing_shift > world_divisor              ) { bad_field = "spacing_shift"; }
		else if(  e.delay_tolerance > world_divisor            ) { bad_field = "delay_tolerance"; }
		else if(  e.max_speed_kmh_of_convoi > 0xFFFF           ) { bad_field = "max_speed_kmh_of_convoi"; }
		else if(  e.balance_speed_kmh_of_convoi > 0xFFFF       ) { bad_field = "balance_speed_kmh_of_convoi"; }
		else if(  e.length_coupling_done > 0xFFFF              ) { bad_field = "length_coupling_done"; }
		if(  bad_field  ) {
			reject(errmsg, "Bad value in schedule file.\n");
			errmsg.printf("stop %u: %s\n", i+1, bad_field);
			return false;
		}
	}

	if(  mode == SCHEDULE_IMPORT_REPLACE_ALL  ) {
		// every stop must exist on this map and be usable by this schedule type
		for(  uint32 i = 0;  i < parsed.get_count();  i++  ) {
			const grund_t *gr = welt->lookup(parsed[i].pos);
			if(  gr == NULL  ||  !target->is_stop_allowed(gr)  ) {
				reject(errmsg, "This stop is not valid on the map.\n");
				errmsg.printf("stop %u: %i,%i,%i\n", i+1, (int)parsed[i].pos.x, (int)parsed[i].pos.y, (int)parsed[i].pos.z);
				return false;
			}
		}
	}
	else {
		// departure times are matched by position, so the stops must be identical
		if(  parsed.get_count() != target->get_count()  ) {
			reject(errmsg, "The stops do not match the current schedule.\n");
			errmsg.printf("%u != %u\n", parsed.get_count(), (uint32)target->get_count());
			return false;
		}
		for(  uint32 i = 0;  i < parsed.get_count();  i++  ) {
			const koord3d &a = parsed[i].pos;
			const koord3d &b = target->at((uint8)i).pos;
			if(  !(a == b)  ) {
				reject(errmsg, "The stops do not match the current schedule.\n");
				errmsg.printf("stop %u: %i,%i,%i != %i,%i,%i\n", i+1,
					(int)a.x, (int)a.y, (int)a.z, (int)b.x, (int)b.y, (int)b.z);
				return false;
			}
		}
	}

	// resolve the line references by name, they are only meaningful inside one world
	bool ambiguous = false;
	linehandle_t group_line = find_line_by_name(player, header.departure_slot_group, ambiguous);
	if(  !header.departure_slot_group.empty()  &&  !group_line.is_bound()  ) {
		warnmsg.append(translator::translate("Could not find the line of the departure slot group.\n"));
		warnmsg.printf("%s\n", header.departure_slot_group.c_str());
	}
	else if(  ambiguous  ) {
		warnmsg.append(translator::translate("More than one line has this name.\n"));
		warnmsg.printf("%s\n", header.departure_slot_group.c_str());
	}

	// -- apply ---------------------------------------------------------------
	if(  mode == SCHEDULE_IMPORT_DEPARTURE_ONLY  ) {
		for(  uint32 i = 0;  i < parsed.get_count();  i++  ) {
			const parsed_entry_t &pe = parsed[i];
			schedule_entry_t &te = target->at((uint8)i);
			te.set_spacing((uint16)pe.spacing, (uint16)pe.spacing_shift, (uint16)pe.delay_tolerance);
			te.set_wait_for_time( (pe.stop_flags & schedule_entry_t::WAIT_FOR_TIME) != 0 );
			te.set_load_before_departure( (pe.stop_flags & schedule_entry_t::LOAD_BEFORE_DEP) != 0 );
		}
		target->set_same_dep_time( (header.schedule_flags & schedule_t::SAME_DEP_TIME) != 0 );
		target->set_departure_slot_group_id(group_line);
		return true;
	}

	bool next_ambiguous = false;
	linehandle_t next_line = find_line_by_name(player, header.next_line, next_ambiguous);
	if(  !header.next_line.empty()  &&  !next_line.is_bound()  ) {
		warnmsg.append(translator::translate("Could not find the next line.\n"));
		warnmsg.printf("%s\n", header.next_line.c_str());
	}

	// Reuse the well tested compact form: sscanf_schedule() also restores the recorded
	// journey/waiting times of the stops that did not move.
	const uint8 old_current_stop = target->get_current_stop();
	cbuffer_t compact;
	const uint32 packed = (uint32)old_current_stop + ((header.schedule_flags & 0xFFu) << 8) + (header.max_speed << 16);
	compact.printf("%u|%u|%u|%d|%u|", packed, (uint32)group_line.get_id(),
		header.additional_base_waiting_time, (int)target->get_type(), next_line.get_id());
	for(  uint32 i = 0;  i < parsed.get_count();  i++  ) {
		const parsed_entry_t &e = parsed[i];
		compact.printf("%i,%i,%i,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u|",
			(int)e.pos.x, (int)e.pos.y, (int)e.pos.z,
			e.minimum_loading, e.waiting_time_shift, e.stop_flags,
			e.max_speed_kmh_of_convoi, e.spacing, e.spacing_shift, e.delay_tolerance,
			e.length_coupling_done, e.maximum_loading, e.balance_speed_kmh_of_convoi);
	}
	if(  !target->sscanf_schedule(compact)  ) {
		reject(errmsg, "Could not apply the schedule.\n");
		return false;
	}
	// keep heading for the stop we were heading for, rather than the exporter's
	target->set_current_stop(old_current_stop);
	return true;
}
