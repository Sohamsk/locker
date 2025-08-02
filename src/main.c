#include "shared_memory.h"
#include "xdg-shell-client-protocol.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

struct prog_state {
	struct wl_display *display;
	struct wl_compositor *compositor;
	struct wl_shm *shm;
	struct wl_shm_pool *pool;
	struct wl_surface *surface;
	struct wl_buffer *buffer;
	struct xdg_wm_base *xdg_base;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;
	uint8_t closed;
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
			     uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel,
			    int32_t width, int32_t height,
			    struct wl_array *states) {
	// idk what i should do here
}

void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {

	struct prog_state *state = data;
	state->closed = 1;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

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
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		state->xdg_base = wl_registry_bind(wl_registry, name,
						   &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(state->xdg_base, &xdg_wm_base_listener,
					 state);
	}
	// printf("Interface: %s,\n version: %d,\n name: %d\n", interface,
	// version, name);
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
				  uint32_t serial) {
	struct prog_state *state = data;
	xdg_surface_ack_configure(xdg_surface, serial);

	wl_surface_attach(state->surface, state->buffer, 0, 0);
	wl_surface_commit(state->surface);
}

struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void reg_handle_global_remove(void *data,
				     struct wl_registry *wl_registry,
				     uint32_t name) {}

static const struct wl_registry_listener reg_listener = {
    .global = reg_handle_global, .global_remove = reg_handle_global_remove};

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
	fprintf(stdout, "Connection established!!\n");
	wl_registry_add_listener(registry, &reg_listener, state);
	wl_display_roundtrip(state->display);
	wl_registry_destroy(registry);
}

void createBuffer(uint32_t width, uint32_t height, uint32_t stride,
		  struct prog_state *state) {
	size_t shm_pool_size = height * stride * 2;

	int fd = allocate_shm_file(shm_pool_size);
	uint8_t *pool_data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE,
				  MAP_SHARED, fd, 0);
	state->pool = wl_shm_create_pool(state->shm, fd, shm_pool_size);

	int index = 0;
	int offset = height * stride * index;
	state->buffer = wl_shm_pool_create_buffer(
	    state->pool, offset, width, height, stride, WL_SHM_FORMAT_XRGB8888);

	// checker pattern
	uint32_t *pixels = (uint32_t *)&pool_data[offset];
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if ((x + y / 8 * 8) % 16 < 8) {
				pixels[y * width + x] = 0xFF666666;
			} else {
				pixels[y * width + x] = 0xFFEEEEEE;
			}
		}
	}

	close(fd);
	munmap(pool_data, shm_pool_size);
	wl_shm_pool_destroy(state->pool);
	wl_shm_destroy(state->shm);
}

int main() {
	struct prog_state state = {0};

	getDisplay(&state);

	uint32_t width = 1920, height = 1080;
	uint32_t stride = width * 4;

	createBuffer(width, height, stride, &state);

	state.surface = wl_compositor_create_surface(state.compositor);
	state.xdg_surface =
	    xdg_wm_base_get_xdg_surface(state.xdg_base, state.surface);
	xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener,
				 &state);
	state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
	xdg_toplevel_set_title(state.xdg_toplevel, "Example client");
	xdg_toplevel_add_listener(state.xdg_toplevel, &xdg_toplevel_listener,
				  &state);
	wl_surface_commit(state.surface);

	while (!state.closed && wl_display_dispatch(state.display)) {
		/* This space deliberately left blank */
	}

	wl_buffer_destroy(state.buffer);
	wl_surface_destroy(state.surface);
	wl_compositor_destroy(state.compositor);
	wl_display_disconnect(state.display);

	return 0;
}
