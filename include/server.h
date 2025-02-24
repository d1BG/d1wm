#include "wlr.h"
#include "output.h"
#include "cursor.h"
#include "toplevel.h"
#include "popup.h"
#include "keyboard.h"

struct d1_server {
    struct wl_display *wl_display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;

    struct wlr_xdg_shell *xdg_shell;
    struct wl_listener new_xdg_toplevel{};
    struct wl_listener new_xdg_popup{};
    struct wl_list toplevels{};

    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;

    struct wlr_seat *seat;
    struct wl_listener new_input{};
    struct wl_listener request_cursor{};
    struct wl_listener request_set_selection{};
    struct wl_list keyboards{};
    enum d1_cursor_mode cursor_mode;
    struct d1_toplevel *grabbed_toplevel{};
    double grab_x{}, grab_y{};
    struct wlr_box grab_geobox{};
    uint32_t resize_edges{};

    struct wlr_output_layout *output_layout;
    struct wl_list outputs{};
    struct wl_listener new_output{};

    d1_server(char *startupcmd);
    ~d1_server();
};

void focus_toplevel(struct d1_toplevel *toplevel);
void server_new_xdg_popup(struct wl_listener *listener, void *data);
void xdg_popup_destroy(struct wl_listener *listener, void *data);
void xdg_popup_commit(struct wl_listener *listener, void *data);
void server_new_xdg_toplevel(struct wl_listener *listener, void *data);
void xdg_toplevel_request_fullscreen(struct wl_listener *listener, void *data);
void xdg_toplevel_request_maximize(struct wl_listener *listener, void *data);
void xdg_toplevel_request_resize(struct wl_listener *listener, void *data);
void xdg_toplevel_request_move(struct wl_listener *listener, void *data);
void begin_interactive(struct d1_toplevel *toplevel, enum d1_cursor_mode mode, uint32_t edges);
void xdg_toplevel_destroy(struct wl_listener *listener, void *data);
void xdg_toplevel_commit(struct wl_listener *listener, void *data);
void xdg_toplevel_unmap(struct wl_listener *listener, void *data);
void xdg_toplevel_map(struct wl_listener *listener, void *data);
void server_new_output(struct wl_listener *listener, void *data);
void output_destroy(struct wl_listener *listener, void *data);
void output_request_state(struct wl_listener *listener, void *data);
void output_frame(struct wl_listener *listener, void *data);

void process_cursor_motion(struct d1_server *server, uint32_t time);
void process_cursor_resize(struct d1_server *server);
void process_cursor_move(struct d1_server *server);
void reset_cursor_mode(struct d1_server *server);

struct d1_toplevel *desktop_toplevel_at(
        struct d1_server *server, double lx, double ly,
        struct wlr_surface **surface, double *sx, double *sy);

void seat_request_set_selection(struct wl_listener *listener, void *data);
void seat_request_cursor(struct wl_listener *listener, void *data);
void server_new_input(struct wl_listener *listener, void *data);
void server_new_pointer(struct d1_server *server, struct wlr_input_device *device);
void server_new_keyboard(struct d1_server *server,struct wlr_input_device *device);
void keyboard_handle_destroy(struct wl_listener *listener, void *data);
void keyboard_handle_key(struct wl_listener *listener, void *data);
bool handle_keybinding(struct d1_server *server, xkb_keysym_t sym);
void keyboard_handle_modifiers(struct wl_listener *listener, void *data);