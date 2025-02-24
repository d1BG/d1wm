#include "server.h"

d1_cursor::d1_cursor(struct d1_server *server) {
	this->server = server;
	this->cursor = server->cursor;
	this->cursor_mgr = server->cursor_mgr;

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