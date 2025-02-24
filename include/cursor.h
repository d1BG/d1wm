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

void process_cursor_motion(struct d1_server *server, uint32_t time);
void process_cursor_resize(struct d1_server *server);
void process_cursor_move(struct d1_server *server);
void reset_cursor_mode(struct d1_server *server);