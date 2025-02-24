#include "server.h"

d1_popup::d1_popup(struct wlr_xdg_popup *popup) {
    this->xdg_popup = popup;

    /* We must add xdg popups to the scene graph so they get rendered. The
     * wlroots scene graph provides a helper for this, but to use it we must
     * provide the proper parent scene node of the xdg popup. To enable this,
     * we always set the user data field of xdg_surfaces to the corresponding
     * scene node. */
    struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(xdg_popup->parent);
    assert(parent != nullptr);
    struct wlr_scene_tree *parent_tree = static_cast<wlr_scene_tree*>(parent->data);
    xdg_popup->base->data = wlr_scene_xdg_surface_create(parent_tree, xdg_popup->base);

    commit.notify = xdg_popup_commit;
    wl_signal_add(&xdg_popup->base->surface->events.commit, &commit);

    destroy.notify = [](wl_listener *listener, void *data) {
        struct d1_popup *popup = wl_container_of(listener, popup, destroy);
        delete popup;
    };
    wl_signal_add(&xdg_popup->events.destroy, &destroy);

}


void xdg_popup_commit(struct wl_listener *listener, void *data) {
    /* Called when a new surface state is committed. */
    struct d1_popup *popup = wl_container_of(listener, popup, commit);

    if (popup->xdg_popup->base->initial_commit) {
        // TODO: Check if popup position is offscreen
        wlr_xdg_surface_schedule_configure(popup->xdg_popup->base);
    }
}

d1_popup::~d1_popup() {
    wl_list_remove(&commit.link);
    wl_list_remove(&destroy.link);
}