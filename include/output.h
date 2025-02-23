#include "wlr.h"

struct tinywl_output {
    struct wl_list link;
    struct d1_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;
};