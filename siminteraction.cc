/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "siminteraction.h"

#include "simversion.h"
#include "dataobj/environment.h"
#include "gui/gui_frame.h"
#include "gui/help_frame.h"
#include "network/network_cmd_ingame.h"
#include "dataobj/scenario.h"
#include "simevent.h"
#include "utils/simrandom.h"
#include "simmenu.h"
#include "player/simplay.h"
#include "simsound.h"
#include "sys/simsys.h"
#include "simticker.h"
#include "gui/simwin.h"
#include "simworld.h"
#include "descriptor/sound_desc.h"
#include "obj/zeiger.h"
#include "display/viewport.h"

#include <math.h>


karte_ptr_t interaction_t::world;

// only the drag movements of this period before the release determine the inertial speed (ms)
static const uint32 INERTIA_SAMPLE_WINDOW = 100;
// no inertia, if the mouse rested this long before the release (ms)
static const uint32 INERTIA_MAX_IDLE = 50;
// the speed decays to 1/e within this time (ms)
static const double INERTIA_TIME_CONSTANT = 325.0;
// speed limits in pixels per ms
// this is subtracted from the release speed, so slow drags hardly glide while fast ones keep most of their speed
static const double INERTIA_SPEED_DEADZONE = 0.5;
static const double INERTIA_START_SPEED = 0.1;
static const double INERTIA_STOP_SPEED = 0.03;
static const double INERTIA_MAX_SPEED = 10.0;


void interaction_t::reset_drag_tracking()
{
	drag_sample_head = 0;
	drag_sample_count = 0;
	drag_start_time = dr_time();
	inertia_active = false;
}


void interaction_t::record_drag(sint16 dx, sint16 dy)
{
	drag_sample_head = (drag_sample_head + 1) % DRAG_SAMPLES;
	drag_samples[drag_sample_head].time = dr_time();
	drag_samples[drag_sample_head].dx = dx;
	drag_samples[drag_sample_head].dy = dy;
	if(  drag_sample_count < DRAG_SAMPLES  ) {
		drag_sample_count++;
	}
}


void interaction_t::start_inertia()
{
	const uint32 now = dr_time();
	const uint8 count = drag_sample_count;
	const uint8 head = drag_sample_head;
	uint32 base_time = drag_start_time;
	reset_drag_tracking();

	if(  !env_t::scroll_inertia  ||  count == 0  ||  now - drag_samples[head].time > INERTIA_MAX_IDLE  ) {
		return;
	}

	// sum up the movements within the window, newest first
	sint32 sum_x = 0, sum_y = 0;
	for(  uint8 i = 0;  i < count;  i++  ) {
		const drag_sample_t &s = drag_samples[(head + DRAG_SAMPLES - i) % DRAG_SAMPLES];
		if(  now - s.time > INERTIA_SAMPLE_WINDOW  ||  i == DRAG_SAMPLES-1  ) {
			// the movements after this one are inside the window
			// (or the older ones are already overwritten)
			base_time = s.time;
			break;
		}
		sum_x += s.dx;
		sum_y += s.dy;
	}

	const double duration = (double)max( (int)(now - base_time), 1 );
	double vx = sum_x / duration;
	double vy = sum_y / duration;
	const double speed = sqrt( vx*vx + vy*vy );
	double new_speed = speed - INERTIA_SPEED_DEADZONE;
	if(  new_speed > INERTIA_MAX_SPEED  ) {
		new_speed = INERTIA_MAX_SPEED;
	}
	if(  new_speed < INERTIA_START_SPEED  ) {
		return;
	}
	vx *= new_speed / speed;
	vy *= new_speed / speed;

	inertia_active = true;
	inertia_vx = vx;
	inertia_vy = vy;
	inertia_rest_x = 0.0;
	inertia_rest_y = 0.0;
	inertia_last_time = now;
}


void interaction_t::step_inertia()
{
	if(  !inertia_active  ) {
		return;
	}
	if(  viewport->get_follow_convoi().is_bound()  ) {
		// following a convoi takes over the view
		inertia_active = false;
		return;
	}

	const uint32 now = dr_time();
	// limit the step after a long stall (e.g. while saving)
	const uint32 dt = min( (int)(now - inertia_last_time), 100 );
	if(  dt == 0  ) {
		return;
	}
	inertia_last_time = now;

	// exact travel distance while the speed decays exponentially during dt
	const double decay = exp( -(double)dt / INERTIA_TIME_CONSTANT );
	const double travel = INERTIA_TIME_CONSTANT * (1.0 - decay);
	inertia_rest_x += inertia_vx * travel;
	inertia_rest_y += inertia_vy * travel;
	inertia_vx *= decay;
	inertia_vy *= decay;

	// only whole pixels are moved, the remainder is carried over
	const sint16 dx = (sint16)inertia_rest_x;
	const sint16 dy = (sint16)inertia_rest_y;
	inertia_rest_x -= dx;
	inertia_rest_y -= dy;
	if(  dx != 0  ||  dy != 0  ) {
		viewport->change_world_position( viewport->get_world_position(), viewport->get_x_off() + dx, viewport->get_y_off() + dy );

		// the map moved below the resting mouse => update the cursor like a mouse move does
		event_t ev(EVENT_MOVE);
		ev.mx = get_mouse_x();
		ev.my = get_mouse_y();
		ev.button_state = 0;
		move_cursor(ev);
	}

	if(  inertia_vx*inertia_vx + inertia_vy*inertia_vy < INERTIA_STOP_SPEED*INERTIA_STOP_SPEED  ) {
		inertia_active = false;
	}
}


void interaction_t::move_view( const event_t &ev )
{
	koord new_ij = viewport->get_world_position();

	const sint16 dx = -(ev.mx-ev.cx) * env_t::scroll_multi;
	const sint16 dy = -(ev.my-ev.cy) * env_t::scroll_multi;
	if(  dx != 0  ||  dy != 0  ) {
		record_drag( dx, dy );
	}

	sint16 new_xoff = viewport->get_x_off() + dx;
	sint16 new_yoff = viewport->get_y_off() + dy;

	// this sets the new position and mark screen dirty
	// => with next refresh we will be at a new location
	viewport->change_world_position( new_ij, new_xoff, new_yoff );

	// move the mouse pointer back to starting location => infinite mouse movement
	if ((ev.mx - ev.cx) != 0 || (ev.my - ev.cy) != 0) {
		if (!env_t::scroll_infinite || !move_pointer(ev.cx, ev.cy)) {
			// fails in finger mode
			change_drag_start(ev.mx - ev.cx, ev.my - ev.cy);
		}
		}
}


void interaction_t::move_cursor( const event_t &ev )
{
	zeiger_t *zeiger = world->get_zeiger();

	if(! zeiger ) {
		// No cursor to move, exit
		return;
	}

	static int mb_alt=0;

	tool_t *tool = world->get_tool(world->get_active_player_nr());

	const koord3d pos = viewport->get_new_cursor_position(scr_coord(ev.mx,ev.my), tool->is_grid_tool());

	if( pos == koord3d::invalid ) {
		zeiger->change_pos(pos);
		return;
	}

	// move cursor
	const koord3d prev_pos = zeiger->get_pos();
	if(  (prev_pos != pos ||  ev.button_state != mb_alt)  ) {

		mb_alt = ev.button_state;

		zeiger->change_pos(pos);

		if (!tool->move_has_effects()) {
			is_dragging = false;
		}
		else if(  !env_t::networkmode  ||  tool->is_move_network_safe(world->get_active_player())) {
			tool->flags = event_get_last_control_shift() | tool_t::WFL_LOCAL;
			if(tool->check_pos( world->get_active_player(), zeiger->get_pos() )==NULL) {
				if(  ev.button_state == 0  ) {
					is_dragging = false;
				}
				else if(ev.ev_class==EVENT_DRAG) {
					if(!is_dragging  &&  prev_pos != koord3d::invalid  &&  tool->check_pos( world->get_active_player(), prev_pos )==NULL) {
						const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), tool->get_id(), tool->get_waytype(), prev_pos);
						if (err == NULL) {
							is_dragging = true;
						}
						else {
							is_dragging = false;
						}
					}
				}
				if (is_dragging) {
					const char* err = world->get_scenario()->is_work_allowed_here(world->get_active_player(), tool->get_id(), tool->get_waytype(), pos);
					if (err == NULL) {
						tool->move( world->get_active_player(), is_dragging, pos );
					}
				}
			}
			tool->flags = 0;
		}

		if(  (ev.button_state&7)==0  ) {
			// time, since mouse got here
			world->set_mouse_rest_time(dr_time());
			world->set_sound_wait_time(AMBIENT_SOUND_INTERVALL); // 13s no movement: play sound
		}
	}
}


void interaction_t::interactive_event( const event_t &ev )
{
	if(ev.ev_class == EVENT_KEYBOARD) {
		DBG_MESSAGE("interaction_t::interactive_event()","Keyboard event with code %d '%c'", ev.ev_code, (ev.ev_code>=32  &&  ev.ev_code<=126) ? ev.ev_code : '?' );

		switch(ev.ev_code) {

			// cursor movements
			case SIM_KEY_UPRIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord::north);
				world->set_dirty();
				break;
			case SIM_KEY_DOWNLEFT:
				viewport->change_world_position(viewport->get_world_position() + koord::south);
				world->set_dirty();
				break;
			case SIM_KEY_UPLEFT:
				viewport->change_world_position(viewport->get_world_position() + koord::west);
				world->set_dirty();
				break;
			case SIM_KEY_DOWNRIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord::east);
				world->set_dirty();
				break;
			case SIM_KEY_RIGHT:
				viewport->change_world_position(viewport->get_world_position() + koord(+1,-1));
				world->set_dirty();
				break;
			case SIM_KEY_DOWN:
				viewport->change_world_position(viewport->get_world_position() + koord(+1,+1));
				world->set_dirty();
				break;
			case SIM_KEY_UP:
				viewport->change_world_position(viewport->get_world_position() + koord(-1,-1));
				world->set_dirty();
				break;
			case SIM_KEY_LEFT:
				viewport->change_world_position(viewport->get_world_position() + koord(-1,+1));
				world->set_dirty();
				break;

			// closing windows
			case 27:
			case 127:
				if( !IS_CONTROL_PRESSED( &ev ) && !IS_SHIFT_PRESSED( &ev ) ) {
					// close topmost win
					destroy_win( win_get_top() );
				}
				break;

			case SIM_KEY_F1:
				if(  gui_frame_t *win = win_get_top()  ) {
					if(  const char *helpfile = win->get_help_filename()  ) {
						help_frame_t::open_help_on( helpfile );
						break;
					}
				}
				world->set_tool( tool_t::dialog_tool[DIALOG_HELP], world->get_active_player() );
				break;

			// just ignore the key
			case 0:
				break;

			// distinguish between backspace and ctrl-H (both keycode==8), and enter and ctrl-M (both keycode==13)
			case 8:
			case 13:
				if(  !IS_CONTROL_PRESSED(&ev)  &&  !IS_SHIFT_PRESSED(&ev)  ) {
					// Control is _not_ pressed => Backspace or Enter pressed.
					if(  ev.ev_code == 8  ) {
						// Backspace
						sound_play(SFX_SELECT,255,TOOL_SOUND);
						destroy_all_win(false);
					}
					// Ignore Enter and Backspace but not Ctrl-H and Ctrl-M
					break;
				}
				/* FALLTHROUGH */

			default:
				{
					bool ok=false;
					FOR(vector_tpl<tool_t*>, const i, tool_t::char_to_tool) {
						if(  i->command_key == ev.ev_code  ) {
							if(  i->command_flags == 0  ||  (ev.ev_key_mod & (SIM_MOD_SHIFT|SIM_MOD_CTRL)) == i->command_flags  ) {
								world->set_tool(i, world->get_active_player());
								ok = true;
								break;
							}
						}
					}
#ifdef STEAM_BUILT
					// Block F12 from bringing up Keyboard Help (for Steam Screenshot) - but still allow F12 to be used if defined in pakset
					if (ev.ev_code==SIM_KEY_F12) {
						ok=true;
					}
#endif
					if(!ok) {
						help_frame_t::open_help_on( "keys.txt" );
					}
				}
				break;
		}
	}

	if(  IS_LEFTRELEASE(&ev)  &&  ev.my < display_get_height() -16 -(TICKER_HEIGHT*ticker::empty())  ) {

		DBG_MESSAGE("interaction_t::interactive_event(event_t &ev)", "calling a tool");

		koord3d pos = world->get_zeiger()->get_pos();
		if(world->is_within_grid_limits(pos.get_2d())) {

			bool suspended = false; // true if execution was suspended, i.e. sent to server
			tool_t *tool = world->get_tool(world->get_active_player_nr());
			player_t *player = world->get_active_player();
			tool->flags = event_get_last_control_shift();
			// first check for visibility etc (needs already right flags)
			const char *err = tool->check_pos( player, pos );
			if (err==NULL) {
				err = world->call_work(tool, player, pos, suspended);
			}
			if (!suspended) {
				// play sound / error message
				world->get_active_player()->tell_tool_result(tool, pos, err);

				// Check if we need to update pointer(zeiger) position.
				if( err == NULL  &&  tool->update_pos_after_use() ) {
					// Cursor might need movement (screen has changed, we get a new one under the mouse pointer)
					const koord3d pos_new = viewport->get_new_cursor_position(scr_coord(ev.mx,ev.my), tool->is_grid_tool());
					world->get_zeiger()->set_pos(pos_new);
				}
			}
			tool->flags = 0;
		}
	}

	// mouse wheel scrolled -> rezoom
	if(  ev.ev_class==EVENT_CLICK  ) {
		if(  ev.ev_code==MOUSE_WHEELUP  ) {
			zoom_view(ev, true);
		}
		else if(  ev.ev_code==MOUSE_WHEELDOWN  ) {
			zoom_view(ev, false);
		}
	}
}

void interaction_t::zoom_view(const event_t &ev, const bool isOut) {
	// use mouse screen pos directly: zeiger may be stale after minimap navigation
	tool_t *tool = world->get_tool(world->get_active_player_nr());
	const koord3d cursor_pos = viewport->get_new_cursor_position(scr_coord(ev.mx,ev.my), tool->is_grid_tool());
	// old screen position of tile under mouse
	const scr_coord s = viewport->get_screen_coord(cursor_pos, koord(0,0));

	if(  !win_change_zoom_factor(isOut)  ) {
		// zoom failed.
		return;
	}

	// calculate offsets such that tile under cursor is still on the same screen position
	if(  cursor_pos!=koord3d::invalid  ) {
		viewport->change_world_position(cursor_pos, koord(0,0), s);
	}

	//and move cursor to the new position under the mouse
	move_cursor(ev);
	
	world->set_dirty();
}


bool interaction_t::process_event( event_t &ev )
{
	if(ev.ev_class==EVENT_SYSTEM  &&  ev.ev_code==SYSTEM_QUIT) {
		// quit the program if this window is closed
		world->stop(true);
		return true;
	}

	if(ev.ev_class==IGNORE_EVENT) {
		// ignore it
		return false;
	}

	if(  ev.ev_class == EVENT_CLICK  ) {
		// any click (also on a window) stops the inertial scrolling and may start a new map drag
		reset_drag_tracking();
	}

	DBG_DEBUG4("interaction_t::process_event", "calling check_pos_win");
	if(check_pos_win(&ev)){
		// The event is shallowed by the GUI, next.
		return false;
	}

	DBG_DEBUG4("interaction_t::process_event", "after check_pos_win");

	// Handle map drag with right-click

	static bool left_drag = false;

	if(IS_RIGHTCLICK(&ev)) {
		display_show_pointer(false);
	}
	else if(IS_RIGHTRELEASE(&ev)) {
		display_show_pointer(true);
		start_inertia();
	}
	else if(IS_RIGHTDRAG(&ev)) {
		// unset following
		world->get_viewport()->set_follow_convoi( convoihandle_t() );
		catch_dragging();
		move_view(ev);
	}
	else if(  (left_drag  ||  world->get_tool(world->get_active_player_nr())->get_id() == (TOOL_QUERY | GENERAL_TOOL))  &&  IS_LEFTDRAG(&ev)  ) {
		/* ok, we have the query tool selected, and we have a left drag or left release event with an actual difference
		 * => move the map */
		if(  !left_drag  ) {
			display_show_pointer(false);
			left_drag = true;
		}
		world->get_viewport()->set_follow_convoi( convoihandle_t() );
		catch_dragging();
		move_view(ev);
		ev.ev_code = IGNORE_EVENT;
	}

	if(  IS_LEFTRELEASE(&ev)  &&  left_drag  ) {
		// show the mouse and swallow this event if we were dragging before
		ev.ev_code = IGNORE_EVENT;
		display_show_pointer(true);
		left_drag = false;
		start_inertia();
	}


	DBG_DEBUG4("interaction_t::process_event", "check if cursor needs movement");


	if( (ev.ev_class==EVENT_DRAG  &&  ev.ev_code==MOUSE_LEFTBUTTON)  ||  (ev.button_state==0  &&  ev.ev_class==EVENT_MOVE)  ||  ev.ev_class==EVENT_RELEASE) {
		move_cursor(ev);
	}

	DBG_DEBUG4("interaction_t::process_event", "calling interactive_event");

	interactive_event(ev);

	DBG_DEBUG4("interaction_t::process_event", "end of event handling");

	return false;
}


void interaction_t::check_events()
{
	event_t ev;

	if(  env_t::networkmode  ) {
		set_random_mode( INTERACTIVE_RANDOM );
	}

	win_poll_event(&ev);

	while (  ev.ev_class != EVENT_NONE ) {

		DBG_DEBUG4("interaction_t::check_events", "called win_poll_event");

		if (process_event(ev)) {
			// We have been asked to stop processing, exit.
			return;
		}

		win_poll_event(&ev);
	}

	step_inertia();

	if(  env_t::networkmode  ) {
		clear_random_mode( INTERACTIVE_RANDOM );
	}
}


interaction_t::interaction_t()
{
	viewport = world->get_viewport();
	is_dragging = false;
	reset_drag_tracking();

	// Requires a world with a view already attached!
	assert(viewport);
}
