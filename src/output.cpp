#include "server.h"

d1_output::d1_output(d1_server *server, struct wlr_output *wlr_output) {
    this->server = server;
    this->wlr_output = wlr_output;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);

    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);

    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode != nullptr) {
        wlr_output_state_set_mode(&state, mode);
    }

    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);

    struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout,
        wlr_output);
    struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);

    /* Sets up a listener for the frame event. */
    frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &frame);

    /* Sets up a listener for the state request event. */
    request_state.notify = output_request_state;
    wl_signal_add(&wlr_output->events.request_state, &request_state);

    /* Sets up a listener for the destroy event. */
    destroy.notify = [](wl_listener *listener, void *data) {
        struct d1_output *output = wl_container_of(listener, output, destroy);
        delete output;
    };
    wl_signal_add(&wlr_output->events.destroy, &destroy);

}

d1_output::~d1_output() {
    wl_list_remove(&frame.link);
    wl_list_remove(&request_state.link);
    wl_list_remove(&destroy.link);
    wl_list_remove(&link);
}

void output_frame(struct wl_listener *listener, void *data) {
    /* This function is called every time an output is ready to display a frame,
     * generally at the output's refresh rate (e.g. 60Hz). */
    struct d1_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
        scene, output->wlr_output);

    /* Render the scene if needed and commit the output */
    wlr_scene_output_commit(scene_output, nullptr);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

void output_request_state(struct wl_listener *listener, void *data) {
    /* This function is called when the backend requests a new state for
     * the output. For example, Wayland and X11 backends request a new mode
     * when the output window is resized. */
    struct d1_output *output = wl_container_of(listener, output, request_state);
    const struct wlr_output_event_request_state *event = static_cast<wlr_output_event_request_state*>(data);
    wlr_output_commit_state(output->wlr_output, event->state);
}