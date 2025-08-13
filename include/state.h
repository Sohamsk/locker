#ifndef HEADER_STATE
#define HEADER_STATE
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <security/pam_ext.h>
#include <security/pam_modules.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

typedef enum {
	AUTH_STATE_LOCKED,
	AUTH_STATE_AUTHENTICATING,
	AUTH_STATE_SUCCESS,
	//    ICON_STATE_FAILED,
	AUTH_STATE_TYPING
} auth_state_t;

struct auth_state {
	pam_handle_t *pamh;
	char *username;
	char *password_buffer;
	size_t password_len;
	uint32_t password_pos;
	auth_state_t current_state;
};

struct prog_state {
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct wl_shm_pool *pool;
	size_t shm_pool_size;
	uint8_t *pool_data;
	struct wl_buffer *buffer;
	struct wl_seat *seat;
	struct wl_keyboard *keyboard;

	struct wl_output *output;
	uint32_t logical_width;
	uint32_t logical_height;
	//  NOTE: there can be multiple outputs each output is analogus to a
	//  screen. On my current machine i only have one output and also for
	//  simplicity i will only bind to one output

	// keyboard stuff
	struct xkb_context *xkb_context;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	// lock stuff
	struct ext_session_lock_manager_v1 *lock_manager;
	struct ext_session_lock_v1 *session_lock;
	struct wl_surface *surface;
	struct ext_session_lock_surface_v1 *lock_surface;
	bool locked;

	// auth state
	struct auth_state auth_state;

	// decay_state
	struct wl_callback *decay_callback;
	uint32_t decay_interval;
	struct timespec last_activity;
	bool decay_enabled;
};
#endif
