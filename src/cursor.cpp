#include "server.h"

d1_cursor::d1_cursor(struct d1_server *server) {
	this->server = server;
	cursor = server->cursor;
	cursor_mgr = server->cursor_mgr;

    cursor_motion.notify = [](wl_listener *listener, void *data) {
    	/* This event is forwarded by the cursor when a pointer emits a _relative_
	 * pointer motion event (i.e. a delta) */
    	d1_cursor *cursor = wl_container_of(listener, cursor, cursor_motion);
    	struct wlr_pointer_motion_event *event = static_cast<wlr_pointer_motion_event*>(data);
    	/* The cursor doesn't move unless we tell it to. The cursor automatically
		 * handles constraining the motion to the output layout, as well as any
		 * special configuration applied for the specific input device which
		 * generated the event. You can pass nullptr for the device if you want to move
		 * the cursor around without any input. */
    	wlr_cursor_move(cursor->server->cursor, &event->pointer->base,
				event->delta_x, event->delta_y);
    	process_cursor_motion(cursor->server, event->time_msec);
    };
    wl_signal_add(&cursor->events.motion, &cursor_motion);

    cursor_motion_absolute.notify = [](struct wl_listener *listener, void *data) {
    	/* This event is forwarded by the cursor when a pointer emits an _absolute_
	 * motion event, from 0..1 on each axis. This happens, for example, when
	 * wlroots is running under a Wayland window rather than KMS+DRM, and you
	 * move the mouse over the window. You could enter the window from any edge,
	 * so we have to warp the mouse there. There is also some hardware which
	 * emits these events. */
    	d1_cursor *cursor = wl_container_of(listener, cursor, cursor_motion_absolute);
    	struct wlr_pointer_motion_absolute_event *event = static_cast<wlr_pointer_motion_absolute_event*>(data);
    	wlr_cursor_warp_absolute(cursor->server->cursor, &event->pointer->base, event->x,
			event->y);
    	process_cursor_motion(cursor->server, event->time_msec);
    };
    wl_signal_add(&cursor->events.motion_absolute, &cursor_motion_absolute);

    cursor_button.notify = [](wl_listener *listener, void *data) {
    	/* This event is forwarded by the cursor when a pointer emits a button
		 * event. */
    	d1_cursor *cursor = wl_container_of(listener, cursor, cursor_button);
    	struct wlr_pointer_button_event *event = static_cast<wlr_pointer_button_event*>(data);
    	/* Notify the client with pointer focus that a button press has occurred */
    	wlr_seat_pointer_notify_button(cursor->server->seat,
				event->time_msec, event->button, event->state);
    	if (event->state == WL_POINTER_BUTTON_STATE_RELEASED) {
    		/* If you released any buttons, we exit interactive move/resize mode. */
    		reset_cursor_mode(cursor->server);
    	} else {
    		/* Focus that client if the button was _pressed_ */
    		double sx, sy;
    		struct wlr_surface *surface = nullptr;
    		struct d1_toplevel *toplevel = desktop_toplevel_at(cursor->server,
					cursor->server->cursor->x, cursor->server->cursor->y, &surface, &sx, &sy);
    		focus_toplevel(toplevel);
    	}
    };
    wl_signal_add(&cursor->events.button, &cursor_button);

    cursor_axis.notify = [](wl_listener *listener, void *data) {
    	/* This event is forwarded by the cursor when a pointer emits an axis event,
		 * for example when you move the scroll wheel. */
    	d1_cursor *cursor = wl_container_of(listener, cursor, cursor_axis);

    	auto *event = static_cast<wlr_pointer_axis_event*>(data);

    	/* Notify the client with pointer focus of the axis event. */
    	wlr_seat_pointer_notify_axis(cursor->server->seat,
				event->time_msec, event->orientation, event->delta,
				event->delta_discrete, event->source, event->relative_direction);
    };
    wl_signal_add(&cursor->events.axis, &cursor_axis);

    cursor_frame.notify = [](wl_listener *listener, void *data) {
    	/* This event is forwarded by the cursor when a pointer emits a frame
	   * event. Frame events are sent after regular pointer events to group
	   * multiple events together. For instance, two axis events may happen at the
	   * same time, in which case a frame event won't be sent in between. */
    	d1_cursor *cursor = wl_container_of(listener, cursor, cursor_frame);
    	/* Notify the client with pointer focus of the frame event. */
    	wlr_seat_pointer_notify_frame(cursor->server->seat);
    };
    wl_signal_add(&cursor->events.frame, &cursor_frame);
}

d1_cursor::~d1_cursor(){
    wl_list_remove(&cursor_motion.link);
    wl_list_remove(&cursor_motion_absolute.link);
    wl_list_remove(&cursor_button.link);
    wl_list_remove(&cursor_axis.link);
    wl_list_remove(&cursor_frame.link);
}


void reset_cursor_mode(struct d1_server *server) {
	/* Reset the cursor mode to passthrough. */
	server->cursor_mode = D1_CURSOR_PASSTHROUGH;
	server->grabbed_toplevel = nullptr;
}

void process_cursor_move(struct d1_server *server) {
	/* Move the grabbed toplevel to the new position. */
	struct d1_toplevel *toplevel = server->grabbed_toplevel;
	wlr_scene_node_set_position(&toplevel->scene_tree->node,
		server->cursor->x - server->grab_x,
		server->cursor->y - server->grab_y);
}

void process_cursor_resize(struct d1_server *server) {
	/*
	 * Resizing the grabbed toplevel can be a little bit complicated, because we
	 * could be resizing from any corner or edge. This not only resizes the
	 * toplevel on one or two axes, but can also move the toplevel if you resize
	 * from the top or left edges (or top-left corner).
	 *
	 * Note that some shortcuts are taken here. In a more fleshed-out
	 * compositor, you'd wait for the client to prepare a buffer at the new
	 * size, then commit any movement that was prepared.
	 */
	struct d1_toplevel *toplevel = server->grabbed_toplevel;
	double border_x = server->cursor->x - server->grab_x;
	double border_y = server->cursor->y - server->grab_y;
	int new_left = server->grab_geobox.x;
	int new_right = server->grab_geobox.x + server->grab_geobox.width;
	int new_top = server->grab_geobox.y;
	int new_bottom = server->grab_geobox.y + server->grab_geobox.height;

	if (server->resize_edges & WLR_EDGE_TOP) {
		new_top = border_y;
		if (new_top >= new_bottom) {
			new_top = new_bottom - 1;
		}
	} else if (server->resize_edges & WLR_EDGE_BOTTOM) {
		new_bottom = border_y;
		if (new_bottom <= new_top) {
			new_bottom = new_top + 1;
		}
	}
	if (server->resize_edges & WLR_EDGE_LEFT) {
		new_left = border_x;
		if (new_left >= new_right) {
			new_left = new_right - 1;
		}
	} else if (server->resize_edges & WLR_EDGE_RIGHT) {
		new_right = border_x;
		if (new_right <= new_left) {
			new_right = new_left + 1;
		}
	}

	struct wlr_box *geo_box = &toplevel->xdg_toplevel->base->geometry;
	wlr_scene_node_set_position(&toplevel->scene_tree->node,
		new_left - geo_box->x, new_top - geo_box->y);

	int new_width = new_right - new_left;
	int new_height = new_bottom - new_top;
	wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, new_width, new_height);
}

void process_cursor_motion(struct d1_server *server, uint32_t time) {
	/* If the mode is non-passthrough, delegate to those functions. */
	if (server->cursor_mode == D1_CURSOR_MOVE) {
		process_cursor_move(server);
		return;
	} else if (server->cursor_mode == D1_CURSOR_RESIZE) {
		process_cursor_resize(server);
		return;
	}

	/* Otherwise, find the toplevel under the pointer and send the event along. */
	double sx, sy;
	struct wlr_seat *seat = server->seat;
	struct wlr_surface *surface = nullptr;
	struct d1_toplevel *toplevel = desktop_toplevel_at(server,
			server->cursor->x, server->cursor->y, &surface, &sx, &sy);
	if (!toplevel) {
		/* If there's no toplevel under the cursor, set the cursor image to a
		 * default. This is what makes the cursor image appear when you move it
		 * around the screen, not over any toplevels. */
		wlr_cursor_set_xcursor(server->cursor, server->cursor_mgr, "default");
	}
	if (surface) {
		/*
		 * Send pointer enter and motion events.
		 *
		 * The enter event gives the surface "pointer focus", which is distinct
		 * from keyboard focus. You get pointer focus by moving the pointer over
		 * a window.
		 *
		 * Note that wlroots will avoid sending duplicate enter/motion events if
		 * the surface already has pointer focus or if the client is already
		 * aware of the coordinates passed.
		 */
		wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
		wlr_seat_pointer_notify_motion(seat, time, sx, sy);
	} else {
		/* Clear pointer focus so future button events and such are not sent to
		 * the last client to have the cursor over it. */
		wlr_seat_pointer_clear_focus(seat);
	}
}
