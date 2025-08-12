#include "state.h"
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Adapted from swaylock: https://github.com/swaywm/swaylock
 * Copyright (c) 2016-2019 Drew DeVault
 * Licensed under MIT - see
 * https://github.com/swaywm/swaylock/blob/master/LICENSE
 * The adaped code starts below and goes on until informed further.
 */
static int handle_conversation(int num_msg, const struct pam_message **msg,
			       struct pam_response **resp, void *data) {
	/* PAM expects an array of responses, one for each message */
	struct prog_state *state = (struct prog_state *)data;
	struct pam_response *pam_reply =
	    calloc(num_msg, sizeof(struct pam_response));
	if (pam_reply == NULL) {
		fprintf(stderr, "Allocation failed 1\n");
		return PAM_ABORT;
	}
	*resp = pam_reply;
	for (int i = 0; i < num_msg; ++i) {
		switch (msg[i]->msg_style) {
		case PAM_PROMPT_ECHO_OFF:
		case PAM_PROMPT_ECHO_ON:
			pam_reply[i].resp = strdup(
			    state->auth_state
				.password_buffer); // PAM clears and frees this
			if (pam_reply[i].resp == NULL) {
				fprintf(stderr, "Allocation failed 2\n");
				return PAM_ABORT;
			}
			break;
		case PAM_ERROR_MSG:
		case PAM_TEXT_INFO:
			break;
		}
	}
	return PAM_SUCCESS;
}
/*
 * The adapted code ends here
 */

static struct pam_conv conv = {
    .conv = handle_conversation,
    .appdata_ptr = NULL, // set at init_pam
};

int init_pam(struct prog_state *state) {
	conv.appdata_ptr = state;
	state->auth_state.password_len = 256;
	state->auth_state.password_pos = 0;
	state->auth_state.password_buffer =
	    calloc(state->auth_state.password_len, sizeof(char));

	const char *user = getenv("USER");
	if (user) {
		state->auth_state.username = strdup(user);
	} else {
		state->auth_state.username = "unknown";
	}

	fprintf(stderr, "Current user: %s\n", state->auth_state.username);

	int ret = pam_start("locker", state->auth_state.username, &conv,
			    &state->auth_state.pamh);
	if (ret != PAM_SUCCESS) {
		return -1;
	}
	return 0;
}

int authenticate_user(struct prog_state *state) {
	struct auth_state *auth_state = &state->auth_state;
	if (!auth_state->pamh) {
		fprintf(stderr, "PAM not init\n");
		return -1;
	}
	fprintf(stderr, "AUTHENTICATING USER\n");

	int ret = pam_authenticate(auth_state->pamh, PAM_SILENT);
	if (ret == PAM_SUCCESS) {
		ret = pam_acct_mgmt(auth_state->pamh, PAM_SILENT);
		if (ret == PAM_SUCCESS) {
			fprintf(stderr,
				"Verified everything auth successful\n");
		} else {
			fprintf(stderr, "account verification failed\n");
			return -1;
		}

	} else {
		fprintf(stderr, "AUTH FAILED\n");
		return -1;
	}
	return 0;
}
