#ifndef HEADER_DRAW
#define HEADER_DRAW

#include <state.h>
#include <stdint.h>

void createBuffer(uint32_t width, uint32_t height, uint32_t stride,
		  struct prog_state *state);
void change_icon_state(struct prog_state *client_state, auth_state_t state);
#endif
