#include "wlr.h"

struct d1_toplevel {
    struct wl_list link;
    struct d1_server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener commit;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;

    d1_toplevel(struct d1_server *server, wlr_xdg_toplevel *wlr_xdg_toplevel);

    ~d1_toplevel();
};

void xdg_toplevel_commit(struct wl_listener *listener, void *data);
void xdg_toplevel_unmap(struct wl_listener *listener, void *data);
void xdg_toplevel_map(struct wl_listener *listener, void *data);

void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
void xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);

void focus_toplevel(struct d1_toplevel *toplevel);
void begin_interactive(struct d1_toplevel *toplevel, enum d1_cursor_mode mode, uint32_t edges);

struct d1_toplevel *desktop_toplevel_at(
        struct d1_server *server, double lx, double ly,
        struct wlr_surface **surface, double *sx, double *sy);