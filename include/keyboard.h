#include "wlr.h"

struct tinywl_keyboard {
    struct wl_list link;
    struct d1_server *server;
    struct wlr_keyboard *wlr_keyboard;

    struct wl_listener modifiers;
    struct wl_listener key;
    struct wl_listener destroy;
};
