/*
	Copyright (c) 2017-2018 ByteBit

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

extern struct RENDER_OPTIONS {
    char name[16];
	int opengl14;
	int color_correction;
	int shadow_entities;
	int ambient_occlusion;
	float render_distance;
	int window_width;
	int window_height;
	unsigned char multisamples;
	int player_arms;
	int fullscreen;
	int greedy_meshing;
	int vsync;
	float mouse_sensitivity;
	int show_news;
	int show_fps;
} settings;

extern struct list config_keys;

struct config_key_pair {
    int internal;
    int def;
    int toggle;
    char name[32];
};

enum {
	CONFIG_TYPE_STRING,
	CONFIG_TYPE_INT,
	CONFIG_TYPE_FLOAT
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

void config_register_key(int internal, int def, const char* name, int toggle);
int config_key_translate(int key, int dir);
struct config_key_pair* config_key(int key);
void config_key_reset_togglestates();
void config_reload(void);
