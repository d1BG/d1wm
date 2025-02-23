#include "wlr.h"

struct tinywl_popup {
    struct wlr_xdg_popup *xdg_popup;
    struct wl_listener commit;
    struct wl_listener destroy;
};