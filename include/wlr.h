#include <cassert>
#include <getopt.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <wayland-server-core.h>
#include <xkbcommon/xkbcommon.h>

#define class class_
#define namespace namespace_
#define delete delete_
#define static

extern "C" {

#define WLR_USE_UNSTABLE 1

#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>

}

#undef class
#undef namespace
#undef static
#undef delete