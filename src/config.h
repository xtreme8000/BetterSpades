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

#ifndef CONFIG_H
#define CONFIG_H

#include "list.h"

struct config_file_entry {
	char section[32];
	char name[32];
	char value[32];
};

extern struct RENDER_OPTIONS {
	char name[16];
	int opengl14;
	int color_correction;
	int shadow_entities;
	int ambient_occlusion;
	float render_distance;
	int window_width;
	int window_height;
	int multisamples;
	int player_arms;
	int fullscreen;
	int greedy_meshing;
	int vsync;
	float mouse_sensitivity;
	int show_news;
	int show_fps;
	int volume;
	int voxlap_models;
	int force_displaylist;
	int invert_y;
	int smooth_fog;
} settings, settings_tmp;

extern struct list config_keys;

struct config_key_pair {
	int internal;
	int def;
	int toggle;
	char name[24];
	char display[24];
	char category[24];
};

enum {
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_INT,
	CONFIG_TYPE_FLOAT,
};

struct config_setting {
	void* value;
	int type;
	int max;
	char name[32];
	char help[32];
	int defaults[8];
	int defaults_length;
};

extern struct list config_settings;

void config_register_key(int internal, int def, const char* name, int toggle, const char* display,
						 const char* category);
int config_key_translate(int key, int dir, int* results);
struct config_key_pair* config_key(int key);
void config_key_reset_togglestates();
void config_reload(void);
void config_save(void);

#endif
