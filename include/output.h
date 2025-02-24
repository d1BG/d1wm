#include "wlr.h"

struct d1_output {
    struct wl_list link;
    struct d1_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
    struct wl_listener request_state;
    struct wl_listener destroy;

    d1_output(d1_server *server);
    ~d1_output();

    static void output_frame(struct wl_listener *listener, void *data);
    static void server_new_output(struct wl_listener *listener, void *data);

};