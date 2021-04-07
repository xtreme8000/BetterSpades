/*
	Copyright (c) 2017-2020 ByteBit

	This file is part of BetterSpades.

	BetterSpades is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	BetterSpades is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "window.h"
#include "file.h"
#include "ini.h"
#include "config.h"
#include "sound.h"
#include "model.h"
#include "camera.h"

struct RENDER_OPTIONS settings, settings_tmp;
struct list config_keys;
struct list config_settings;

struct list config_file;

static void config_sets(const char* section, const char* name, const char* value) {
	for(int k = 0; k < list_size(&config_file); k++) {
		struct config_file_entry* e = list_get(&config_file, k);
		if(strcmp(e->name, name) == 0) {
			strncpy(e->value, value, sizeof(e->value) - 1);
			return;
		}
	}
	struct config_file_entry e;
	strncpy(e.section, section, sizeof(e.section) - 1);
	strncpy(e.name, name, sizeof(e.name) - 1);
	strncpy(e.value, value, sizeof(e.value) - 1);
	list_add(&config_file, &e);
}

static void config_seti(const char* section, const char* name, int value) {
	char tmp[32];
	sprintf(tmp, "%i", value);
	config_sets(section, name, tmp);
}

static void config_setf(const char* section, const char* name, float value) {
	char tmp[32];
	sprintf(tmp, "%0.6f", value);
	config_sets(section, name, tmp);
}

void config_save() {
	kv6_rebuild_complete();

	config_sets("client", "name", settings.name);
	config_seti("client", "xres", settings.window_width);
	config_seti("client", "yres", settings.window_height);
	config_seti("client", "windowed", !settings.fullscreen);
	config_seti("client", "multisamples", settings.multisamples);
	config_seti("client", "greedy_meshing", settings.greedy_meshing);
	config_seti("client", "vsync", settings.vsync);
	config_setf("client", "mouse_sensitivity", settings.mouse_sensitivity);
	config_seti("client", "show_news", settings.show_news);
	config_seti("client", "vol", settings.volume);
	config_seti("client", "show_fps", settings.show_fps);
	config_seti("client", "voxlap_models", settings.voxlap_models);
	config_seti("client", "force_displaylist", settings.force_displaylist);
	config_seti("client", "inverty", settings.invert_y);
	config_seti("client", "smooth_fog", settings.smooth_fog);
	config_seti("client", "ambient_occlusion", settings.ambient_occlusion);
	config_setf("client", "camera_fov", settings.camera_fov);
	config_seti("client", "hold_down_sights", settings.hold_down_sights);

	for(int k = 0; k < list_size(&config_keys); k++) {
		struct config_key_pair* e = list_get(&config_keys, k);
		if(strlen(e->name) > 0)
			config_seti("controls", e->name, e->def);
	}

	void* f = file_open("config.ini", "w");
	if(f) {
		char last_section[32] = {0};
		for(int k = 0; k < list_size(&config_file); k++) {
			struct config_file_entry* e = list_get(&config_file, k);
			if(strcmp(e->section, last_section) != 0) {
				file_printf(f, "\r\n[%s]\r\n", e->section);
				strcpy(last_section, e->section);
			}
			file_printf(f, "%s", e->name);
			for(int l = 0; l < 31 - strlen(e->name); l++)
				file_printf(f, " ");
			file_printf(f, "= %s\r\n", e->value);
		}
		file_close(f);
	}
}

static int config_read_key(void* user, const char* section, const char* name, const char* value) {
	struct config_file_entry e;
	strncpy(e.section, section, sizeof(e.section) - 1);
	strncpy(e.name, name, sizeof(e.name) - 1);
	strncpy(e.value, value, sizeof(e.value) - 1);
	list_add(&config_file, &e);

	if(!strcmp(section, "client")) {
		if(!strcmp(name, "name")) {
			strcpy(settings.name, value);
		} else if(!strcmp(name, "xres")) {
			settings.window_width = atoi(value);
		} else if(!strcmp(name, "yres")) {
			settings.window_height = atoi(value);
		} else if(!strcmp(name, "windowed")) {
			settings.fullscreen = !atoi(value);
		} else if(!strcmp(name, "multisamples")) {
			settings.multisamples = atoi(value);
		} else if(!strcmp(name, "greedy_meshing")) {
			settings.greedy_meshing = atoi(value);
		} else if(!strcmp(name, "vsync")) {
			settings.vsync = atoi(value);
		} else if(!strcmp(name, "mouse_sensitivity")) {
			settings.mouse_sensitivity = atof(value);
		} else if(!strcmp(name, "show_news")) {
			settings.show_news = atoi(value);
		} else if(!strcmp(name, "vol")) {
			settings.volume = max(min(atoi(value), 10), 0);
			sound_volume(settings.volume / 10.0F);
		} else if(!strcmp(name, "show_fps")) {
			settings.show_fps = atoi(value);
		} else if(!strcmp(name, "voxlap_models")) {
			settings.voxlap_models = atoi(value);
		} else if(!strcmp(name, "force_displaylist")) {
			settings.force_displaylist = atoi(value);
		} else if(!strcmp(name, "inverty")) {
			settings.invert_y = atoi(value);
		} else if(!strcmp(name, "smooth_fog")) {
			settings.smooth_fog = atoi(value);
		} else if(!strcmp(name, "ambient_occlusion")) {
			settings.ambient_occlusion = atoi(value);
		} else if(!strcmp(name, "camera_fov")) {
			settings.camera_fov = fmax(fmin(atof(value), CAMERA_MAX_FOV), CAMERA_DEFAULT_FOV);
		} else if(!strcmp(name, "hold_down_sights")) {
			settings.hold_down_sights = atoi(value);
		}
	}
	if(!strcmp(section, "controls")) {
		for(int k = 0; k < list_size(&config_keys); k++) {
			struct config_key_pair* key = list_get(&config_keys, k);
			if(!strcmp(name, key->name)) {
				log_debug("found override for %s, from %i to %i", key->name, key->def, atoi(value));
				key->def = strtol(value, NULL, 0);
				break;
			}
		}
	}
	return 1;
}

void config_register_key(int internal, int def, const char* name, int toggle, const char* display,
						 const char* category) {
	struct config_key_pair key;
	key.internal = internal;
	key.def = def;
	key.original = def;
	key.toggle = toggle;
	if(display)
		strncpy(key.display, display, sizeof(key.display) - 1);
	else
		*key.display = 0;

	if(name)
		strncpy(key.name, name, sizeof(key.name) - 1);
	else
		*key.name = 0;

	if(category)
		strncpy(key.category, category, sizeof(key.category) - 1);
	else
		*key.category = 0;
	list_add(&config_keys, &key);
}

int config_key_translate(int key, int dir, int* results) {
	int count = 0;

	for(int k = 0; k < list_size(&config_keys); k++) {
		struct config_key_pair* a = list_get(&config_keys, k);

		if(dir && a->internal == key) {
			if(results)
				results[count] = a->def;
			count++;
		} else if(!dir && a->def == key) {
			if(results)
				results[count] = a->internal;
			count++;
		}
	}

	return count;
}

struct config_key_pair* config_key(int key) {
	for(int k = 0; k < list_size(&config_keys); k++) {
		struct config_key_pair* a = list_get(&config_keys, k);
		if(a->internal == key)
			return a;
	}
	return NULL;
}

void config_key_reset_togglestates() {
	for(int k = 0; k < list_size(&config_keys); k++) {
		struct config_key_pair* a = list_get(&config_keys, k);
		if(a->toggle)
			window_pressed_keys[a->internal] = 0;
	}
}

static int config_key_cmp(const void* a, const void* b) {
	const struct config_key_pair* A = (const struct config_key_pair*)a;
	const struct config_key_pair* B = (const struct config_key_pair*)b;

	int cmp = strcmp(A->category, B->category);
	return cmp ? cmp : strcmp(A->display, B->display);
}

static void config_label_pixels(char* buffer, size_t length, int value, size_t index) {
	if(value == 800 || value == 600) {
		snprintf(buffer, length, "default: %ipx", value);
	} else {
		snprintf(buffer, length, "%ipx", value);
	}
}

static void config_label_vsync(char* buffer, size_t length, int value, size_t index) {
	if(value == 0) {
		snprintf(buffer, length, "disabled");
	} else if(value == 1) {
		snprintf(buffer, length, "enabled");
	} else {
		snprintf(buffer, length, "max %i fps", value);
	}
}

static void config_label_msaa(char* buffer, size_t length, int value, size_t index) {
	if(index == 0) {
		snprintf(buffer, length, "No MSAA");
	} else {
		snprintf(buffer, length, "%ix MSAA", value);
	}
}

void config_reload() {
	if(!list_created(&config_file))
		list_create(&config_file, sizeof(struct config_file_entry));
	else
		list_clear(&config_file);

	if(!list_created(&config_keys))
		list_create(&config_keys, sizeof(struct config_key_pair));
	else
		list_clear(&config_keys);

#ifdef USE_SDL
	config_register_key(WINDOW_KEY_UP, SDLK_w, "move_forward", 0, "Forward", "Movement");
	config_register_key(WINDOW_KEY_DOWN, SDLK_s, "move_backward", 0, "Backward", "Movement");
	config_register_key(WINDOW_KEY_LEFT, SDLK_a, "move_left", 0, "Left", "Movement");
	config_register_key(WINDOW_KEY_RIGHT, SDLK_d, "move_right", 0, "Right", "Movement");
	config_register_key(WINDOW_KEY_SPACE, SDLK_SPACE, "jump", 0, "Jump", "Movement");
	config_register_key(WINDOW_KEY_SPRINT, SDLK_LSHIFT, "sprint", 0, "Sprint", "Movement");
	config_register_key(WINDOW_KEY_SHIFT, SDLK_LSHIFT, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_CURSOR_UP, SDLK_UP, "cube_color_up", 0, "Color up", "Block");
	config_register_key(WINDOW_KEY_CURSOR_DOWN, SDLK_DOWN, "cube_color_down", 0, "Color down", "Block");
	config_register_key(WINDOW_KEY_CURSOR_LEFT, SDLK_LEFT, "cube_color_left", 0, "Color left", "Block");
	config_register_key(WINDOW_KEY_CURSOR_RIGHT, SDLK_RIGHT, "cube_color_right", 0, "Color right", "Block");
	config_register_key(WINDOW_KEY_BACKSPACE, SDLK_BACKSPACE, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_TOOL1, SDLK_1, "tool_spade", 0, "Select spade", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL2, SDLK_2, "tool_block", 0, "Select block", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL3, SDLK_3, "tool_gun", 0, "Select gun", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL4, SDLK_4, "tool_grenade", 0, "Select grenade", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TAB, SDLK_TAB, "view_score", 0, "Score", "Information");
	config_register_key(WINDOW_KEY_ESCAPE, SDLK_ESCAPE, "quit_game", 0, "Quit", "Game");
	config_register_key(WINDOW_KEY_ESCAPE, SDLK_AC_BACK, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_MAP, SDLK_m, "view_map", 1, "Map", "Information");
	config_register_key(WINDOW_KEY_CROUCH, SDLK_LCTRL, "crouch", 0, "Crouch", "Movement");
	config_register_key(WINDOW_KEY_SNEAK, SDLK_v, "sneak", 0, "Sneak", "Movement");
	config_register_key(WINDOW_KEY_ENTER, SDLK_RETURN, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F1, SDLK_F1, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F2, SDLK_F2, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F3, SDLK_F3, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F4, SDLK_F4, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_YES, SDLK_y, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_YES, SDLK_z, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_NO, SDLK_n, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_VOLUME_UP, SDLK_KP_PLUS, "volume_up", 0, "Volume up", "Game");
	config_register_key(WINDOW_KEY_VOLUME_DOWN, SDLK_KP_MINUS, "volume_down", 0, "Volume down", "Game");
	config_register_key(WINDOW_KEY_V, SDLK_v, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_RELOAD, SDLK_r, "reload", 0, "Reload", "Tools & Weapons");
	config_register_key(WINDOW_KEY_CHAT, SDLK_t, "chat_global", 0, "Chat", "Game");
	config_register_key(WINDOW_KEY_FULLSCREEN, SDLK_F11, "fullscreen", 0, "Fullscreen", "Game");
	config_register_key(WINDOW_KEY_SCREENSHOT, SDLK_F5, "screenshot", 0, "Screenshot", "Information");
	config_register_key(WINDOW_KEY_CHANGETEAM, SDLK_COMMA, "change_team", 0, "Team select", "Game");
	config_register_key(WINDOW_KEY_CHANGEWEAPON, SDLK_PERIOD, "change_weapon", 0, "Gun select", "Tools & Weapons");
	config_register_key(WINDOW_KEY_PICKCOLOR, SDLK_e, "cube_color_sample", 0, "Pick color", "Block");
	config_register_key(WINDOW_KEY_COMMAND, SDLK_SLASH, "chat_command", 0, "Command", "Game");
	config_register_key(WINDOW_KEY_HIDEHUD, SDLK_F6, "hide_hud", 1, "Hide HUD", "Game");
	config_register_key(WINDOW_KEY_LASTTOOL, SDLK_q, "last_tool", 0, "Last tool", "Tools & Weapons");
	config_register_key(WINDOW_KEY_NETWORKSTATS, SDLK_F12, "network_stats", 1, "Network stats", "Information");
	config_register_key(WINDOW_KEY_SAVE_MAP, SDLK_F9, "save_map", 0, "Save map", "Game");
#endif

#ifdef USE_GLFW
	config_register_key(WINDOW_KEY_UP, GLFW_KEY_W, "move_forward", 0, "Forward", "Movement");
	config_register_key(WINDOW_KEY_DOWN, GLFW_KEY_S, "move_backward", 0, "Backward", "Movement");
	config_register_key(WINDOW_KEY_LEFT, GLFW_KEY_A, "move_left", 0, "Left", "Movement");
	config_register_key(WINDOW_KEY_RIGHT, GLFW_KEY_D, "move_right", 0, "Right", "Movement");
	config_register_key(WINDOW_KEY_SPACE, GLFW_KEY_SPACE, "jump", 0, "Jump", "Movement");
	config_register_key(WINDOW_KEY_SPRINT, GLFW_KEY_LEFT_SHIFT, "sprint", 0, "Sprint", "Movement");
	config_register_key(WINDOW_KEY_SHIFT, GLFW_KEY_LEFT_SHIFT, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_CURSOR_UP, GLFW_KEY_UP, "cube_color_up", 0, "Color up", "Block");
	config_register_key(WINDOW_KEY_CURSOR_DOWN, GLFW_KEY_DOWN, "cube_color_down", 0, "Color down", "Block");
	config_register_key(WINDOW_KEY_CURSOR_LEFT, GLFW_KEY_LEFT, "cube_color_left", 0, "Color left", "Block");
	config_register_key(WINDOW_KEY_CURSOR_RIGHT, GLFW_KEY_RIGHT, "cube_color_right", 0, "Color right", "Block");
	config_register_key(WINDOW_KEY_BACKSPACE, GLFW_KEY_BACKSPACE, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_TOOL1, GLFW_KEY_1, "tool_spade", 0, "Select spade", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL2, GLFW_KEY_2, "tool_block", 0, "Select block", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL3, GLFW_KEY_3, "tool_gun", 0, "Select gun", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TOOL4, GLFW_KEY_4, "tool_grenade", 0, "Select grenade", "Tools & Weapons");
	config_register_key(WINDOW_KEY_TAB, GLFW_KEY_TAB, "view_score", 0, "Score", "Information");
	config_register_key(WINDOW_KEY_ESCAPE, GLFW_KEY_ESCAPE, "quit_game", 0, "Quit", "Game");
	config_register_key(WINDOW_KEY_MAP, GLFW_KEY_M, "view_map", 1, "Map", "Information");
	config_register_key(WINDOW_KEY_CROUCH, GLFW_KEY_LEFT_CONTROL, "crouch", 0, "Crouch", "Movement");
	config_register_key(WINDOW_KEY_SNEAK, GLFW_KEY_V, "sneak", 0, "Sneak", "Movement");
	config_register_key(WINDOW_KEY_ENTER, GLFW_KEY_ENTER, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F1, GLFW_KEY_F1, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F2, GLFW_KEY_F2, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F3, GLFW_KEY_F3, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_F4, GLFW_KEY_F4, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_YES, GLFW_KEY_Y, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_YES, GLFW_KEY_Z, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_NO, GLFW_KEY_N, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_VOLUME_UP, GLFW_KEY_KP_ADD, "volume_up", 0, "Volume up", "Game");
	config_register_key(WINDOW_KEY_VOLUME_DOWN, GLFW_KEY_KP_SUBTRACT, "volume_down", 0, "Volume down", "Game");
	config_register_key(WINDOW_KEY_V, GLFW_KEY_V, NULL, 0, NULL, NULL);
	config_register_key(WINDOW_KEY_RELOAD, GLFW_KEY_R, "reload", 0, "Reload", "Tools & Weapons");
	config_register_key(WINDOW_KEY_CHAT, GLFW_KEY_T, "chat_global", 0, "Chat", "Game");
	config_register_key(WINDOW_KEY_FULLSCREEN, GLFW_KEY_F11, "fullscreen", 0, "Fullscreen", "Game");
	config_register_key(WINDOW_KEY_SCREENSHOT, GLFW_KEY_F5, "screenshot", 0, "Screenshot", "Game");
	config_register_key(WINDOW_KEY_CHANGETEAM, GLFW_KEY_COMMA, "change_team", 0, "Team select", "Game");
	config_register_key(WINDOW_KEY_CHANGEWEAPON, GLFW_KEY_PERIOD, "change_weapon", 0, "Gun select", "Tools & Weapons");
	config_register_key(WINDOW_KEY_PICKCOLOR, GLFW_KEY_E, "cube_color_sample", 0, "Pick color", "Block");
	config_register_key(WINDOW_KEY_COMMAND, GLFW_KEY_SLASH, "chat_command", 0, "Command", "Game");
	config_register_key(WINDOW_KEY_HIDEHUD, GLFW_KEY_F6, "hide_hud", 1, "Hide HUD", "Game");
	config_register_key(WINDOW_KEY_LASTTOOL, GLFW_KEY_Q, "last_tool", 0, "Last tool", "Tools & Weapons");
	config_register_key(WINDOW_KEY_NETWORKSTATS, GLFW_KEY_F12, "network_stats", 1, "Network stats", "Information");
	config_register_key(WINDOW_KEY_SAVE_MAP, GLFW_KEY_F9, "save_map", 0, "Save map", "Game");
#endif

	list_sort(&config_keys, config_key_cmp);

	char* s = file_load("config.ini");
	if(s) {
		ini_parse_string(s, config_read_key, NULL);
		free(s);
	}

	if(!list_created(&config_settings))
		list_create(&config_settings, sizeof(struct config_setting));
	else
		list_clear(&config_settings);

	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = settings_tmp.name,
				 .type = CONFIG_TYPE_STRING,
				 .max = sizeof(settings.name) - 1,
				 .name = "Name",
				 .help = "Ingame player name",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.mouse_sensitivity,
				 .type = CONFIG_TYPE_FLOAT,
				 .min = 0,
				 .max = INT_MAX,
				 .name = "Mouse sensitivity",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.camera_fov,
				 .type = CONFIG_TYPE_FLOAT,
				 .min = CAMERA_DEFAULT_FOV,
				 .max = CAMERA_MAX_FOV,
				 .name = "Camera FOV",
				 .help = "Field of View in degrees",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.volume,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 10,
				 .name = "Volume",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.window_width,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = INT_MAX,
				 .name = "Game width",
				 .defaults = 640,
				 800,
				 854,
				 1024,
				 1280,
				 1920,
				 3840,
				 .defaults_length = 7,
				 .help = "Default: 800",
				 .label_callback = config_label_pixels,
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.window_height,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = INT_MAX,
				 .name = "Game height",
				 .defaults = 480,
				 600,
				 720,
				 768,
				 1024,
				 1080,
				 2160,
				 .defaults_length = 7,
				 .help = "Default: 600",
				 .label_callback = config_label_pixels,
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.vsync,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = INT_MAX,
				 .name = "V-Sync",
				 .help = "Limits your game's fps",
				 .defaults = 0,
				 1,
				 60,
				 120,
				 144,
				 240,
				 .defaults_length = 6,
				 .label_callback = config_label_vsync,
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.fullscreen,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .name = "Fullscreen",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.multisamples,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 16,
				 .name = "Multisamples",
				 .help = "Smooth out block edges",
				 .defaults = 0,
				 2,
				 4,
				 8,
				 16,
				 .defaults_length = 5,
				 .label_callback = config_label_msaa,
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.voxlap_models,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "Render models like in voxlap",
				 .name = "Voxlap models",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.hold_down_sights,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "Only aim while pressing RMB",
				 .name = "Hold down sights",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.greedy_meshing,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "Join similar mesh faces",
				 .name = "Greedy meshing",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.force_displaylist,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "Enable this on buggy drivers",
				 .name = "Force Displaylist",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.smooth_fog,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "Enable this on buggy drivers",
				 .name = "Smooth fog",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.ambient_occlusion,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .help = "(won't work with greedy mesh)",
				 .name = "Ambient occlusion",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.show_fps,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .name = "Show fps",
				 .help = "Show current fps and ping ingame",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.invert_y,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .name = "Invert y",
				 .help = "Invert vertical mouse movement",
			 });
	list_add(&config_settings,
			 &(struct config_setting) {
				 .value = &settings_tmp.show_news,
				 .type = CONFIG_TYPE_INT,
				 .min = 0,
				 .max = 1,
				 .name = "Show news",
				 .help = "Open the bns news on exit",
			 });
}
