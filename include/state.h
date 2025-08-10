#ifndef HEADER_STATE
#define HEADER_STATE
#include <stdint.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct prog_state {
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct wl_shm_pool *pool;
	struct wl_buffer *buffer;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;
	struct wl_output *output;
	//  NOTE: there can be multiple outputs each output is analogus to a
	//  screen. On my current machine i only have one output and also for
	//  simplicity i will only bind to one output

	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	struct ext_session_lock_manager_v1 *lock_manager;
	struct ext_session_lock_v1 *session_lock;
	struct wl_surface *surface;
	struct ext_session_lock_surface_v1 *lock_surface;
	uint8_t locked;
};
#endif
