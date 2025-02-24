#include "wlr.h"

struct d1_keyboard {
    struct wl_list link;
    struct d1_server *server;
    struct wlr_keyboard *wlr_keyboard;

    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;

    d1_keyboard(d1_server *server,struct wlr_input_device *device);
    ~d1_keyboard();
};

void keyboard_handle_key(struct wl_listener *listener, void *data);
bool handle_keybinding(struct d1_server *server, xkb_keysym_t sym);
void keyboard_handle_modifiers(struct wl_listener *listener, void *data);
