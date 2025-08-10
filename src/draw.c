#include "shared_memory.h"
#include "state.h"
#include <cairo.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wordexp.h>

static void drawImage(uint32_t width, uint32_t height, uint32_t stride,
		      void *pixels) {
	cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(
	    pixels, CAIRO_FORMAT_ARGB32, width, height, stride);

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

		double scale_x = (double)width / img_width;
		double scale_y = (double)height / img_height;

		cairo_save(cr);
		cairo_scale(cr, scale_x, scale_y);
		cairo_set_source_surface(cr, image, 0, 0);
		cairo_paint(cr);
		cairo_restore(cr);
	} else {
		fprintf(stderr, "failed to get lock_screen wallpaper\n");
		cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		cairo_paint(cr);
	}

	cairo_surface_destroy(image);
	cairo_destroy(cr);
	cairo_surface_destroy(cairo_surface);
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
	    state->pool, offset, width, height, stride, WL_SHM_FORMAT_ARGB8888);

	uint32_t *pixels = (uint32_t *)&pool_data[offset];
	drawImage(width, height, stride, pixels);

	close(fd);
	munmap(pool_data, shm_pool_size);
}
