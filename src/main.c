#include "auth.h"
#include "draw.h"
#include "ext-session-lock-v1-protocol.h"
#include "state.h"
#include <assert.h>
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

static void output_geometry(void *data, struct wl_output *wl_output, int32_t x,
			    int32_t y, int32_t physical_width,
			    int32_t physical_height, int32_t subpixel,
			    const char *make, const char *model,
			    int32_t transform) {
	fprintf(stderr, "Physical screen widthxheight: %dx%d mm\n",
		physical_width, physical_height);
}

static void output_mode(void *data, struct wl_output *wl_output, uint32_t flags,
			int32_t width, int32_t height, int32_t refresh) {
	struct prog_state *output = data;
	if (flags & WL_OUTPUT_MODE_CURRENT) {
		output->physical_width = width;
		output->physical_height = height;
		fprintf(stderr, "Screen resolution: %dx%d pixels\n", width,
			height);
	}
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
};

static void wl_keyboard_listener_keymap(void *data,
					struct wl_keyboard *wl_keyboard,
					uint32_t format, int32_t fd,
					uint32_t size) {
	struct prog_state *client_state = data;
	assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

	char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	assert(map_shm != MAP_FAILED);

	struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
	    client_state->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
	    XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_shm, size);
	close(fd);

	struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
	xkb_keymap_unref(client_state->xkb_keymap);
	xkb_state_unref(client_state->xkb_state);
	client_state->xkb_keymap = xkb_keymap;
	client_state->xkb_state = xkb_state;
}

static void wl_keyboard_listener_enter(void *data,
				       struct wl_keyboard *wl_keyboard,
				       uint32_t serial,
				       struct wl_surface *surface,
				       struct wl_array *keys) {
	struct prog_state *client_state = data;
	fprintf(stderr, "keyboard enter; keys pressed are:\n");
	uint32_t *key;
	wl_array_for_each(key, keys) {
		char buf[128];
		xkb_keysym_t sym = xkb_state_key_get_one_sym(
		    client_state->xkb_state, *key + 8);
		xkb_keysym_get_name(sym, buf, sizeof(buf));
		fprintf(stderr, "sym: %-12s (%d), ", buf, sym);
		xkb_state_key_get_utf8(client_state->xkb_state, *key + 8, buf,
				       sizeof(buf));
		fprintf(stderr, "utf8: '%s'\n", buf);
	}
}

static void wl_keyboard_listener_key(void *data,
				     struct wl_keyboard *wl_keyboard,
				     uint32_t serial, uint32_t time,
				     uint32_t key, uint32_t state) {
	struct prog_state *client_state = data;
	struct auth_state *auth_state = &client_state->auth_state;
	uint32_t keycode = key + 8;
	xkb_keysym_t sym =
	    xkb_state_key_get_one_sym(client_state->xkb_state, keycode);

	if (state != WL_KEYBOARD_KEY_STATE_PRESSED)
		return;

	if (sym == XKB_KEY_Escape) {
		memset(auth_state->password_buffer, 0,
		       auth_state->password_len);
		auth_state->password_pos = 0;
		fprintf(stderr, "Cleared password\n");
	} else if (sym == XKB_KEY_Return) {
		fprintf(stderr, "return\n");
		if (auth_state->password_pos > 0) {
			auth_state->password_buffer[auth_state->password_pos] =
			    '\0';
		}
		if (authenticate_user(client_state) == 0) {
			ext_session_lock_v1_unlock_and_destroy(
			    client_state->session_lock);
			client_state->session_lock = NULL;
			client_state->locked = 0;
			auth_state->auth_success = 1;
			wl_display_roundtrip(client_state->display);
		};
		memset(auth_state->password_buffer, 0,
		       auth_state->password_len);
		auth_state->password_pos = 0;
	} else if (sym == XKB_KEY_BackSpace) {
		fprintf(stderr, "Backspace\n");
		if (auth_state->password_pos > 0) {
			auth_state->password_pos--;
			auth_state->password_buffer[auth_state->password_pos] =
			    '\0';
		}
	} else {
		char buf[8];

		int len = xkb_state_key_get_utf8(client_state->xkb_state,
						 keycode, buf, sizeof(buf));

		if (len > 0 && auth_state->password_pos + len <
				   auth_state->password_len - 1) {
			memcpy(&auth_state
				    ->password_buffer[auth_state->password_pos],
			       buf, len);
			auth_state->password_pos += len;
		} else {
			fprintf(stderr, "password too big for buffer or not a "
					"typable character\n");
		}
	}
}

static void wl_keyboard_listener_leave(void *data,
				       struct wl_keyboard *wl_keyboard,
				       uint32_t serial,
				       struct wl_surface *surface) {
	fprintf(stderr, "keyboard leave\n");
}

static void
wl_keyboard_listener_modifiers(void *data, struct wl_keyboard *wl_keyboard,
			       uint32_t serial, uint32_t mods_depressed,
			       uint32_t mods_latched, uint32_t mods_locked,
			       uint32_t group) {
	struct prog_state *client_state = data;
	xkb_state_update_mask(client_state->xkb_state, mods_depressed,
			      mods_latched, mods_locked, 0, 0, group);
}

static void wl_keyboard_listener_repeat_info(void *data,
					     struct wl_keyboard *wl_keyboard,
					     int32_t rate, int32_t delay) {
	// noop
}

struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_listener_keymap,
    .enter = wl_keyboard_listener_enter,
    .leave = wl_keyboard_listener_leave,
    .key = wl_keyboard_listener_key,
    .modifiers = wl_keyboard_listener_modifiers,
    .repeat_info = wl_keyboard_listener_repeat_info,
};

void wl_seat_listener_capabilities(void *data, struct wl_seat *wl_seat,
				   uint32_t capabilities) {
	struct prog_state *state = data;
	//  TODO: make a keyboard
	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
	    state->keyboard == NULL) {
		state->keyboard = wl_seat_get_keyboard(state->seat);
		wl_keyboard_add_listener(state->keyboard, &wl_keyboard_listener,
					 state);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
		   state->keyboard != NULL) {
		wl_keyboard_release(state->keyboard);
		state->keyboard = NULL;
	}
}

void wl_seat_listener_name(void *data, struct wl_seat *wl_seat,
			   const char *name) {
	//  NOTE: intentionally left blank
}

static struct wl_seat_listener wl_seat_listener = {
    .capabilities = wl_seat_listener_capabilities,
    .name = wl_seat_listener_name};

static void reg_handle_global(void *data, struct wl_registry *wl_registry,
			      uint32_t name, const char *interface,
			      uint32_t version) {

	struct prog_state *state = data;
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		state->compositor = wl_registry_bind(
		    wl_registry, name, &wl_compositor_interface, 4);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		state->shm =
		    wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface,
			  ext_session_lock_manager_v1_interface.name) == 0) {
		state->lock_manager =
		    wl_registry_bind(wl_registry, name,
				     &ext_session_lock_manager_v1_interface, 1);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		state->seat =
		    wl_registry_bind(wl_registry, name, &wl_seat_interface, 7);
		wl_seat_add_listener(state->seat, &wl_seat_listener, state);
	} else if (strcmp(interface, wl_output_interface.name) == 0) {
		state->output = wl_registry_bind(wl_registry, name,
						 &wl_output_interface, 1);
		wl_output_add_listener(state->output, &output_listener, state);
	}
	// printf("Interface: %s,\n version: %d,\n name: %d\n", interface,
	// version, name);
}

static void reg_handle_global_remove(void *data,
				     struct wl_registry *wl_registry,
				     uint32_t name) {
	/* no operation */
}

static const struct wl_registry_listener reg_listener = {
    .global = reg_handle_global,
    .global_remove = reg_handle_global_remove,
};

void getDisplay(struct prog_state *state) {
	state->display = wl_display_connect(NULL);
	if (state->display == NULL) {
		fprintf(stderr, "Failed to connect to wayland display!!\n");
		exit(EXIT_FAILURE);
	}
	struct wl_registry *registry = wl_display_get_registry(state->display);
	if (registry == NULL) {
		fprintf(stderr,
			"Failed to get registry from wayland display!!\n");
		exit(EXIT_FAILURE);
	}
	// fprintf(stdout, "Connection established!!\n");
	wl_registry_add_listener(registry, &reg_listener, state);
	wl_display_roundtrip(state->display);
	wl_registry_destroy(registry);
}

void lock_locked(void *data, struct ext_session_lock_v1 *ext_session_lock_v1) {
	struct prog_state *state = data;
	state->locked = 1;
}

void lock_finished(void *data,
		   struct ext_session_lock_v1 *ext_session_lock_v1) {
	fprintf(stderr, "failed to lock session\n");
	exit(EXIT_FAILURE);
}

struct ext_session_lock_v1_listener lock_listener = {
    .locked = lock_locked,
    .finished = lock_finished,
};

void lock_surface_configure(
    void *data, struct ext_session_lock_surface_v1 *ext_session_lock_surface_v1,
    uint32_t serial, uint32_t width, uint32_t height) {
	fprintf(stderr, "Lock surface Configure called: %dx%d\n", width,
		height);
	struct prog_state *state = data;

	uint32_t stride = width * 4;

	if (!state->buffer) {
		createBuffer(width, height, stride, state);
		if (state->buffer) {
			fprintf(stderr, "Buffer created successfully\n");
		} else {
			fprintf(stderr, "Buffer creation failed!\n");
			exit(EXIT_FAILURE);
		}
	}

	wl_surface_attach(state->surface, state->buffer, 0, 0);
	ext_session_lock_surface_v1_ack_configure(ext_session_lock_surface_v1,
						  serial);
	wl_surface_commit(state->surface);
}

struct ext_session_lock_surface_v1_listener lock_surface_listener = {
    .configure = lock_surface_configure,
};

int main() {
	struct prog_state state = {0};
	if (init_pam(&state) != 0) {
		fprintf(stderr, "PAM start failed!!\n");
		exit(2);
	} else
		fprintf(stderr, "Initialized PAM\n");

	state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	getDisplay(&state);

	state.session_lock =
	    ext_session_lock_manager_v1_lock(state.lock_manager);
	ext_session_lock_v1_add_listener(state.session_lock, &lock_listener,
					 &state);

	wl_display_roundtrip(state.display);

	state.surface = wl_compositor_create_surface(state.compositor);
	if (!state.surface) {
		fprintf(stderr, "surface is null\n");
		exit(2);
	}
	state.lock_surface = ext_session_lock_v1_get_lock_surface(
	    state.session_lock, state.surface, state.output);
	ext_session_lock_surface_v1_add_listener(
	    state.lock_surface, &lock_surface_listener, &state);
	// wl_surface_commit(state.surface);

	wl_display_roundtrip(state.display);

	fprintf(stderr, "locked: %d\n", state.locked);
	while (state.locked) {
		if (wl_display_dispatch(state.display) < 0) {
			fprintf(stderr, "wl_display_dispatch() failed\n");
			ext_session_lock_v1_unlock_and_destroy(
			    state.session_lock);
			break;
		}
	}

	//  NOTE: Clear all memory maybe make a function to clean shit when
	//  exiting
	memset(state.auth_state.password_buffer, 0,
	       state.auth_state.password_len);
	free(state.auth_state.password_buffer);
	ext_session_lock_surface_v1_destroy(state.lock_surface);
	ext_session_lock_manager_v1_destroy(state.lock_manager);
	wl_shm_pool_destroy(state.pool);
	wl_shm_destroy(state.shm);
	wl_buffer_destroy(state.buffer);
	wl_surface_destroy(state.surface);
	wl_compositor_destroy(state.compositor);
	wl_display_disconnect(state.display);

	return 0;
}
