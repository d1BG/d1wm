#include "server.h"
#include <string>

d1_server::d1_server(char *startupcmd) {
    wl_display = wl_display_create();
    backend = wlr_backend_autocreate(wl_display_get_event_loop(wl_display), nullptr);

    if (backend == nullptr) {
        wlr_log(WLR_ERROR, "failed to create wlr_backend");
        return;
    }

    renderer = wlr_renderer_autocreate(backend);
    if (renderer == nullptr) {
        wlr_log(WLR_ERROR, "failed to create wlr_renderer");
        return;
    }

    allocator = wlr_allocator_autocreate(backend, renderer);
    if (allocator == nullptr) {
        wlr_log(WLR_ERROR, "failed to create wlr_allocator");
        return;
    }
	// TODO: split this into shm and dmabuf inits
    wlr_renderer_init_wl_display(renderer, wl_display);

    wlr_compositor_create(wl_display, 5, renderer);
    wlr_subcompositor_create(wl_display);
    wlr_data_device_manager_create(wl_display);

    output_layout = wlr_output_layout_create(wl_display);

	wl_list_init(&outputs);
	new_output.notify = server_new_output;
	wl_signal_add(&backend->events.new_output, &new_output);

    scene = wlr_scene_create();
    scene_layout = wlr_scene_attach_output_layout(scene, output_layout);

	wl_list_init(&toplevels);
	xdg_shell = wlr_xdg_shell_create(wl_display, 6);
	new_xdg_toplevel.notify = server_new_xdg_toplevel;
	wl_signal_add(&xdg_shell->events.new_toplevel, &new_xdg_toplevel);
	new_xdg_popup.notify = server_new_xdg_popup;
	wl_signal_add(&xdg_shell->events.new_popup, &new_xdg_popup);

    cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(cursor, output_layout);

    cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
    cursor_mode = D1_CURSOR_PASSTHROUGH;
	new d1_cursor(this);

    seat = wlr_seat_create(wl_display, "seat0");

	wl_list_init(&keyboards);
	new_input.notify = server_new_input;
	wl_signal_add(&backend->events.new_input, &new_input);
	request_cursor.notify = seat_request_cursor;
	wl_signal_add(&seat->events.request_set_cursor, &request_cursor);
	request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&seat->events.request_set_selection, &request_set_selection);

	// Avoid using "wayland-0" as display socket
	std::string socket;
	for (unsigned int i = 1; i <= 32; i++) {
		socket = "wayland-" + std::to_string(i);
		int ret = wl_display_add_socket(wl_display, socket.c_str());
		if (!ret) {
			break;
		} else {
			wlr_log(WLR_ERROR, "wl_display_add_socket for %s returned %d: skipping", socket.c_str(), ret);
		}
	}

	if (!socket.c_str()) {
		wlr_backend_destroy(backend);
		return;
	}

    if (!wlr_backend_start(backend)) {
        wlr_backend_destroy(backend);
        wl_display_destroy(wl_display);
        return;
    }

    setenv("WAYLAND_DISPLAY", socket.c_str(), true);
    wlr_log(WLR_INFO, "Running Wayland compositor on WAYLAND_DISPLAY=%s",socket.c_str());

	if (startupcmd) {
		if (fork() == 0) {
			execl("/bin/sh", "/bin/sh", "-c", startupcmd, static_cast<void *>(nullptr));
		}
	}


    wl_display_run(wl_display);

}

d1_server::~d1_server() {
    wl_display_destroy_clients(wl_display);

    wl_list_remove(&new_xdg_toplevel.link);
    wl_list_remove(&new_xdg_popup.link);

    wl_list_remove(&new_input.link);
    wl_list_remove(&request_cursor.link);
    wl_list_remove(&request_set_selection.link);

    wl_list_remove(&new_output.link);

    wlr_scene_node_destroy(&scene->tree.node);
    wlr_xcursor_manager_destroy(cursor_mgr);
    wlr_cursor_destroy(cursor);
    wlr_allocator_destroy(allocator);
    wlr_renderer_destroy(renderer);
    wlr_backend_destroy(backend);
    wl_display_destroy(wl_display);
}

void server_new_pointer(struct d1_server *server, struct wlr_input_device *device) {
	// TODO: Libinput
	wlr_cursor_attach_input_device(server->cursor, device);
}

void server_new_input(struct wl_listener *listener, void *data) {
	/* This event is raised by the backend when a new input device becomes
	 * available. */
	// TODO: Touchpad and tablet Support
	struct d1_server *server =
		wl_container_of(listener, server, new_input);
	struct wlr_input_device *device = static_cast<wlr_input_device*>(data);
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:
		new d1_keyboard(server, device);
		break;
	case WLR_INPUT_DEVICE_POINTER:
		server_new_pointer(server, device);
		break;
	default:
		break;
	}
	/* We need to let the wlr_seat know what our capabilities are, which is
	 * communicated to the client. In TinyWL we always have a cursor, even if
	 * there are no pointer devices, so we always include that capability. */
	uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
	if (!wl_list_empty(&server->keyboards)) {
		caps |= WL_SEAT_CAPABILITY_KEYBOARD;
	}
	wlr_seat_set_capabilities(server->seat, caps);
}

void seat_request_cursor(struct wl_listener *listener, void *data) {
	struct d1_server *server = wl_container_of(
			listener, server, request_cursor);
	/* This event is raised by the seat when a client provides a cursor image */
	struct wlr_seat_pointer_request_set_cursor_event *event =
		static_cast<wlr_seat_pointer_request_set_cursor_event*>(data);
	struct wlr_seat_client *focused_client =
		server->seat->pointer_state.focused_client;
	/* This can be sent by any client, so we check to make sure this one is
	 * actually has pointer focus first. */
	if (focused_client == event->seat_client) {
		/* Once we've vetted the client, we can tell the cursor to use the
		 * provided surface as the cursor image. It will set the hardware cursor
		 * on the output that it's currently on and continue to do so as the
		 * cursor moves between outputs. */
		wlr_cursor_set_surface(server->cursor, event->surface,
				event->hotspot_x, event->hotspot_y);
	}
}

void seat_request_set_selection(struct wl_listener *listener, void *data) {
	/* This event is raised by the seat when a client wants to set the selection,
	 * usually when the user copies something. wlroots allows compositors to
	 * ignore such requests if they so choose, but in d1wm we always honor
	 */
	struct d1_server *server = wl_container_of(
			listener, server, request_set_selection);
	struct wlr_seat_request_set_selection_event *event = static_cast<wlr_seat_request_set_selection_event*>(data);
	wlr_seat_set_selection(server->seat, event->source, event->serial);
}

void server_new_output(struct wl_listener *listener, void *data) {
	struct d1_server *server = wl_container_of(listener, server, new_output);
	struct wlr_output *wlr_output = static_cast<struct wlr_output*>(data);

	new d1_output(server, wlr_output);
}

void server_new_xdg_toplevel(struct wl_listener *listener, void *data) {
	/* This event is raised when a client creates a new toplevel (application window). */
	struct d1_server *server = wl_container_of(listener, server, new_xdg_toplevel);
	struct wlr_xdg_toplevel *xdg_toplevel = static_cast<wlr_xdg_toplevel*>(data);

	new d1_toplevel(server, xdg_toplevel);
}

void server_new_xdg_popup(struct wl_listener *listener, void *data) {
	/* This event is raised when a client creates a new popup. */
	struct wlr_xdg_popup *xdg_popup = static_cast<wlr_xdg_popup*>(data);

	new d1_popup(xdg_popup);

}