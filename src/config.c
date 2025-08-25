#include <config.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tomlc17.h>

void readOrDefaultConfig(const char *filePath, struct prog_state *state) {
	state->auth_state.current_state = AUTH_STATE_LOCKED;
	char *buffer = 0;
	int64_t length = 0;

	FILE *config = fopen(filePath, "rb");
	if (config) {
		fseek(config, 0, SEEK_END);
		length = ftell(config);
		fseek(config, 0, SEEK_SET);
		buffer = malloc(length);
		if (buffer) {
			fread(buffer, 1, length, config);
		}
		fclose(config);
	} else {

		fprintf(stderr, "cannot read file\n");
	}

	if (buffer) {
		toml_result_t result = toml_parse(buffer, strlen(buffer));

		toml_datum_t host =
		    toml_seek(result.toptab, "auth.decay_enabled");
		if (host.type != TOML_UNKNOWN) {
			state->decay_enabled = host.u.boolean;
		} else {
			state->decay_enabled = true;
		}
		host = toml_seek(result.toptab, "auth.decay_interval");
		if (host.type != TOML_UNKNOWN) {
			state->decay_interval = host.u.int64;
		} else {
			state->decay_interval = 10;
		}
		host = toml_seek(result.toptab, "wallpaper.path");
		if (host.type != TOML_UNKNOWN) {
			state->wallpaper_path = strdup(host.u.s);
		} else {
			state->wallpaper_path = strdup("");
		}
	}
}
