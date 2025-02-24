#include "wlr.h"

enum d1_cursor_mode {
    D1_CURSOR_PASSTHROUGH,
    D1_CURSOR_MOVE,
    D1_CURSOR_RESIZE,
};

struct d1_cursor {
    struct d1_server *server;

    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;

    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;

    d1_cursor(d1_server *server);
    ~d1_cursor();
};

void server_cursor_frame(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);