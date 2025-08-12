#include "shared_memory.h"
#include "state.h"
#include <cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wordexp.h>

static char *getIconAccState(auth_state_t state) {
	switch (state) {
	case AUTH_STATE_LOCKED:
		return "";
	case AUTH_STATE_SUCCESS:
		return "";
	case AUTH_STATE_TYPING:
		return "";
	case AUTH_STATE_AUTHENTICATING:
		return "";
	}
	fprintf(stderr, "Some invalid state for auth_state_t\n");
	return "";
}

static void drawLock(struct prog_state *state, cairo_t *cr) {
	char *text = getIconAccState(state->auth_state.current_state);
	fprintf(stderr, "drawing: %s\n", text);
	cairo_text_extents_t extents;

	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_select_font_face(cr, "JetBrainsMono Nerd Font",
			       CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 50);

	cairo_text_extents(cr, text, &extents);

	double x = state->physical_width / 2.0 -
		   (extents.width / 2.0 + extents.x_bearing);
	double y = state->physical_height / 2.0 -
		   (extents.height / 2.0 + extents.y_bearing) + 200;

	cairo_move_to(cr, x, y);
	cairo_show_text(cr, text);
}

static void drawImage(struct prog_state *state, uint32_t logical_width,
		      uint32_t logical_height, uint32_t stride, void *pixels) {
	cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
	    pixels, CAIRO_FORMAT_ARGB32, logical_width, logical_height, stride);

	cairo_t *cr = cairo_create(cairo_surface);
	cairo_surface_t *image = NULL;
	wordexp_t result;

	//  TODO: i need to pass the wallpaper path as a command line input. ( i
	//  can put the path in the prog_state)
	if (wordexp("~/Pictures/lock_screen.png", &result,
		    WRDE_NOCMD | WRDE_SHOWERR) == 0) {
		image = cairo_image_surface_create_from_png(result.we_wordv[0]);
	}
	wordfree(&result);

	if (image && cairo_surface_status(image) == CAIRO_STATUS_SUCCESS) {
		uint32_t img_width = cairo_image_surface_get_width(image);
		uint32_t img_height = cairo_image_surface_get_height(image);

		double scale_x = (double)logical_width / img_width;
		double scale_y = (double)logical_height / img_height;

		cairo_save(cr);
		cairo_scale(cr, scale_x, scale_y);
		cairo_set_source_surface(cr, image, 0, 0);
		cairo_paint(cr);
		//		cairo_restore(cr);
	} else {
		fprintf(stderr, "failed to get lock_screen wallpaper\n");
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		cairo_paint(cr);
	}

	drawLock(state, cr);

	cairo_surface_destroy(image);
	cairo_destroy(cr);
	cairo_surface_destroy(cairo_surface);
}

void createBuffer(uint32_t width, uint32_t height, uint32_t stride,
		  struct prog_state *state) {
	size_t shm_pool_size = height * stride * 2;

	int fd = allocate_shm_file(shm_pool_size);
	state->pool_data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, 0);
	state->pool = wl_shm_create_pool(state->shm, fd, shm_pool_size);

	int index = 0;
	int offset = height * stride * index;
	state->buffer = wl_shm_pool_create_buffer(
	    state->pool, offset, width, height, stride, WL_SHM_FORMAT_ARGB8888);

	uint32_t *pixels = (uint32_t *)&state->pool_data[offset];
	drawImage(state, width, height, stride, pixels);

	close(fd);
}

void redraw_surface(struct prog_state *state) {
	fprintf(stderr, "request redraw of surface\n");
	uint32_t stride = state->logical_width * 4;
	if (!state->buffer) {
		createBuffer(state->logical_width, state->logical_height,
			     stride, state);
		wl_surface_attach(state->surface, state->buffer, 0, 0);
		wl_surface_commit(state->surface);
		fprintf(stderr,
			"successful redraw of surface with new buffer\n");
		return;
	}

	int index = 0;
	int offset = state->logical_height * stride * index;

	uint32_t *pixels = (uint32_t *)&state->pool_data[offset];
	drawImage(state, state->logical_width, state->logical_height, stride,
		  pixels);
	wl_surface_damage_buffer(state->surface, 0, 0, state->logical_width,
				 state->logical_height);
	wl_surface_commit(state->surface);
	fprintf(stderr, "successful redraw of surface\n");
}

void change_icon_state(struct prog_state *client_state, auth_state_t state) {
	if (client_state->auth_state.current_state != state) {
		client_state->auth_state.current_state = state;

		fprintf(stderr, "icon state:%d\n", state);
		redraw_surface(client_state);
	}
}
