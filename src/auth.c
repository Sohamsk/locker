#include "state.h"
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <stdlib.h>
#include <string.h>

static int pam_conv(int num_msg, const struct pam_message **msg,
		    struct pam_response **resp, void *appdata_ptr) {
	struct prog_state *state = (struct prog_state *)appdata_ptr;
	struct pam_response *reply = calloc(1, sizeof(struct pam_response));

	if (!reply) {
		return PAM_BUF_ERR;
	}

	for (int i = 0; i < num_msg; i++) {
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
			reply[i].resp =
			    strdup(state->auth_state.password_buffer);
			break;
		case PAM_PROMPT_ECHO_ON:
			break;
		case PAM_ERROR_MSG:
			break;
		case PAM_TEXT_INFO:
			break;
		default:
			free(reply);
			return PAM_CONV_ERR;
		}
	}

	*resp = reply;
	return PAM_SUCCESS;
}

static struct pam_conv conv = {
    .conv = pam_conv,
    .appdata_ptr = NULL, // set at init
};

int init_pam(struct prog_state *state) {
	conv.appdata_ptr = state;

	state->auth_state.username = getenv("USER");
	if (!state->auth_state.username) {
		state->auth_state.username = "unknown";
	}

	int ret = pam_start("locker", state->auth_state.username, &conv,
			    &state->auth_state.pamh);
	if (ret != PAM_SUCCESS) {
		return -1;
	}
	return 0;
}
