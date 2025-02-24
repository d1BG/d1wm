#include "wlr.h"

struct d1_popup {
    struct wlr_xdg_popup *xdg_popup;

    struct wl_listener commit;
    struct wl_listener destroy;

    d1_popup(struct wlr_xdg_popup *popup);
    ~d1_popup();
};

void xdg_popup_destroy(struct wl_listener *listener, void *data);
void xdg_popup_commit(struct wl_listener *listener, void *data);