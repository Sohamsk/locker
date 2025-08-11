#ifndef HEADER_AUTH
#define HEADER_AUTH
#include "state.h"
int init_pam(struct prog_state *state);
int authenticate_user(struct prog_state *state);
#endif
