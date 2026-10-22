/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMINTERACTION_H
#define SIMINTERACTION_H

#include "simtypes.h"


class karte_ptr_t;
class viewport_t;
struct event_t;

/**
 * User interaction class, it collects and processes all user and system events from the OS.
 * @brief Event manager of the game.
 * @see karte_t::interactive
 */
class interaction_t
{
private:
	/**
	 * The associated world.
	 */
	static karte_ptr_t world;

	/**
	 * Associated viewport of the world, this is a cached value.
	 * @note Since a world in our project just has one viewport, we cache its value.
	 */
	viewport_t *viewport;

	/**
	 * Processes a mouse event that's moving the camera.
	 */
	void move_view(const event_t &ev);

	/**
	 * Processes a cursor movement event, related to the tool pointer in-map.
	 * @see zeiger_t
	 */
	void move_cursor(const event_t &ev);

	/**
	 * Processes a user event on the map, like a keyclick, or a mouse event.
	 */
	void interactive_event(const event_t &ev);
	
	/**
	 * Zoom in/out the view.
	 */
	void zoom_view(const event_t &ev, const bool isOut);

	/**
	 * Processes a single event.
	 * @return true if we need to stop processing events.
	 */
	bool process_event( event_t &ev );

	bool is_dragging;

	/**
	 * Recent map drag movements, used to estimate the speed when the drag is released.
	 */
	struct drag_sample_t {
		uint32 time;
		sint16 dx, dy;
	};
	enum { DRAG_SAMPLES = 64 };
	drag_sample_t drag_samples[DRAG_SAMPLES];
	uint8 drag_sample_head;
	uint8 drag_sample_count;
	uint32 drag_start_time;

	/// inertial scrolling after the drag was released: speed in pixels per ms
	bool inertia_active;
	double inertia_vx, inertia_vy;
	double inertia_rest_x, inertia_rest_y;
	uint32 inertia_last_time;

	/**
	 * Forgets the recorded drag movements and stops the inertial scrolling.
	 */
	void reset_drag_tracking();

	/**
	 * Records a map drag movement of (dx,dy) pixels.
	 */
	void record_drag(sint16 dx, sint16 dy);

	/**
	 * Called when a map drag is released, starts the inertial scrolling if the drag was fast enough.
	 */
	void start_inertia();

	/**
	 * Moves the map according to the inertial scrolling speed and decelerates it.
	 */
	void step_inertia();

public:
	/**
	 * Processes all the pending system events.
	 * @brief Message Pump
	 */
	void check_events();

	/**
	 * @note Requires world() with it's viewport already attached!
	 */
	interaction_t();
};

#endif
