#include "server.h"

d1_toplevel::d1_toplevel(d1_server *server, wlr_xdg_toplevel *wlr_xdg_toplevel) {
	this->server = server;
	this->xdg_toplevel = wlr_xdg_toplevel;

	scene_tree =
	wlr_scene_xdg_surface_create(&server->scene->tree, xdg_toplevel->base);

	scene_tree->node.data = this;
	xdg_toplevel->base->data = scene_tree;

  	/* Listen to the various events it can emit */
	map.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_toplevel->base->surface->events.map, &map);

	unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_toplevel->base->surface->events.unmap, &unmap);

	commit.notify = xdg_toplevel_commit;
	wl_signal_add(&xdg_toplevel->base->surface->events.commit, &commit);

	destroy.notify = [](wl_listener *listener, void *data) {
		d1_toplevel *toplevel = wl_container_of(listener, toplevel, destroy);
		delete toplevel;
	};
	wl_signal_add(&xdg_toplevel->events.destroy, &destroy);

	/* cotd */
	request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&xdg_toplevel->events.request_move, &request_move);
	request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&xdg_toplevel->events.request_resize, &request_resize);
	request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&xdg_toplevel->events.request_maximize, &request_maximize);
	request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&xdg_toplevel->events.request_fullscreen, &request_fullscreen);
}

d1_toplevel::~d1_toplevel() {
	wl_list_remove(&map.link);
	wl_list_remove(&unmap.link);
	wl_list_remove(&commit.link);
	wl_list_remove(&destroy.link);
	wl_list_remove(&request_move.link);
	wl_list_remove(&request_resize.link);
	wl_list_remove(&request_maximize.link);
	wl_list_remove(&request_fullscreen.link);
}

void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	struct d1_toplevel *toplevel = wl_container_of(listener, toplevel, map);
	wl_list_insert(&toplevel->server->toplevels, &toplevel->link);
	focus_toplevel(toplevel);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
	/* Called when the surface is unmapped, and should no longer be shown. */
	struct d1_toplevel *toplevel = wl_container_of(listener, toplevel, unmap);

	/* Reset the cursor mode if the grabbed toplevel was unmapped. */
	if (toplevel == toplevel->server->grabbed_toplevel) {
		reset_cursor_mode(toplevel->server);
	}

	wl_list_remove(&toplevel->link);
}

void xdg_toplevel_commit(struct wl_listener *listener, void *data) {
	/* Called when a new surface state is committed. */
	struct d1_toplevel *toplevel = wl_container_of(listener, toplevel, commit);

	if (toplevel->xdg_toplevel->base->initial_commit) {
		/* When a xdg_surface performs an initial commit, the compositor must
		 * reply with a configure so the client can map the surface. tinywl
		 * configures the xdg_toplevel with 0,0 size to let the client pick the
		 * dimensions itself. */
		wlr_xdg_toplevel_set_size(toplevel->xdg_toplevel, 0, 0);
	}
}

void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct d1_toplevel *toplevel = wl_container_of(listener, toplevel, request_move);
	begin_interactive(toplevel, D1_CURSOR_MOVE, 0);
}

void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct wlr_xdg_toplevel_resize_event *event = static_cast<wlr_xdg_toplevel_resize_event*>(data);
	struct d1_toplevel *toplevel = wl_container_of(listener, toplevel, request_resize);
	begin_interactive(toplevel, D1_CURSOR_RESIZE, event->edges);
}

void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on client-side
	 * decorations. tinywl doesn't support maximization, but to conform to
	 * xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply.
	 * However, if the request was sent before an initial commit, we don't do
	 * anything and let the client finish the initial surface setup. */
	struct d1_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_maximize);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	}
}

void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
	/* Just as with request_maximize, we must send a configure here. */
	struct d1_toplevel *toplevel =
		wl_container_of(listener, toplevel, request_fullscreen);
	if (toplevel->xdg_toplevel->base->initialized) {
		wlr_xdg_surface_schedule_configure(toplevel->xdg_toplevel->base);
	}
}