#include "server.h"

d1_keyboard::d1_keyboard(d1_server *server, struct wlr_input_device *device) {
    this->server = server;
    wlr_keyboard = wlr_keyboard_from_input_device(device);

    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, nullptr,
        XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(wlr_keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

    modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&wlr_keyboard->events.modifiers, &modifiers);

    key.notify = keyboard_handle_key;
    wl_signal_add(&wlr_keyboard->events.key, &key);

    destroy.notify = [](wl_listener *listener, void *data) {
        struct d1_keyboard *keyboard = wl_container_of(listener, keyboard, destroy);
        delete keyboard;
    };
    wl_signal_add(&device->events.destroy, &destroy);

    wlr_seat_set_keyboard(server->seat, wlr_keyboard);

    /* And add the keyboard to our list of keyboards */
    wl_list_insert(&server->keyboards, &link);
}

d1_keyboard::~d1_keyboard() {
    wl_list_remove(&modifiers.link);
    wl_list_remove(&key.link);
    wl_list_remove(&destroy.link);
    wl_list_remove(&link);
}

void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    /* This event is raised when a modifier key, such as shift or alt, is
     * pressed. We simply communicate this to the client. */
    struct d1_keyboard *keyboard =
        wl_container_of(listener, keyboard, modifiers);
    /*
     * A seat can only have one keyboard, but this is a limitation of the
     * Wayland protocol - not wlroots. We assign all connected keyboards to the
     * same seat. You can swap out the underlying wlr_keyboard like this and
     * wlr_seat handles this transparently.
     */
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
    /* Send modifiers to the client. */
    wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
        &keyboard->wlr_keyboard->modifiers);
}

void keyboard_handle_key(struct wl_listener *listener, void *data) {
    /* This event is raised when a key is pressed or released. */
    struct d1_keyboard *keyboard =
        wl_container_of(listener, keyboard, key);
    struct d1_server *server = keyboard->server;
    struct wlr_keyboard_key_event *event = static_cast<wlr_keyboard_key_event*>(data);
    struct wlr_seat *seat = server->seat;

    /* Translate libinput keycode -> xkbcommon */
    uint32_t keycode = event->keycode + 8;
    /* Get a list of keysyms based on the keymap for this keyboard */
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(
            keyboard->wlr_keyboard->xkb_state, keycode, &syms);

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
    if ((modifiers & WLR_MODIFIER_ALT) &&
            event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        /* If alt is held down and this button was _pressed_, we attempt to
         * process it as a compositor keybinding. */
        for (int i = 0; i < nsyms; i++) {
            handled = handle_keybinding(server, syms[i]);
        }
            }

    if (!handled) {
        /* Otherwise, we pass it along to the client. */
        wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
        wlr_seat_keyboard_notify_key(seat, event->time_msec,
            event->keycode, event->state);
    }
}

bool handle_keybinding(struct d1_server *server, xkb_keysym_t sym) {

    switch (sym) {
        case XKB_KEY_Escape:
            wl_display_terminate(server->wl_display);
        return true;
        case XKB_KEY_F1:
            /* Cycle to the next toplevel */
                if (wl_list_length(&server->toplevels) < 2) {
                    return true;
                }
        struct d1_toplevel *next_toplevel =
            wl_container_of(server->toplevels.prev, next_toplevel, link);
        focus_toplevel(next_toplevel);
        return true;


    }
    return false;

}