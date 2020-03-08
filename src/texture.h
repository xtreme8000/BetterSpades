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

#ifndef TEXTURE_H
#define TEXTURE_H

struct texture {
	int width, height;
	int texture_id;
	unsigned char* pixels;
};

extern struct texture texture_splash;
extern struct texture texture_minimap;
extern struct texture texture_gradient;

extern struct texture texture_health;
extern struct texture texture_block;
extern struct texture texture_grenade;
extern struct texture texture_ammo_semi;
extern struct texture texture_ammo_smg;
extern struct texture texture_ammo_shotgun;

extern struct texture texture_color_selection;

extern struct texture texture_zoom_semi;
extern struct texture texture_zoom_smg;
extern struct texture texture_zoom_shotgun;

extern struct texture texture_white;
extern struct texture texture_target;
extern struct texture texture_indicator;

extern struct texture texture_player;
extern struct texture texture_medical;
extern struct texture texture_intel;
extern struct texture texture_command;
extern struct texture texture_tracer;

extern struct texture texture_ui_wait;
extern struct texture texture_ui_join;
extern struct texture texture_ui_reload;
extern struct texture texture_ui_bg;
extern struct texture texture_ui_input;
extern struct texture texture_ui_box_empty;
extern struct texture texture_ui_box_check;
extern struct texture texture_ui_arrow;
extern struct texture texture_ui_arrow2;
extern struct texture texture_ui_flags;
extern struct texture texture_ui_alert;
extern struct texture texture_ui_joystick;
extern struct texture texture_ui_knob;

#define TEXTURE_FILTER_NEAREST 0
#define TEXTURE_FILTER_LINEAR 1

void texture_flag_offset(const char* country, float* u, float* v);
void texture_filter(struct texture* t, int filter);
void texture_init(void);
int texture_create(struct texture* t, char* filename);
int texture_create_buffer(struct texture* t, int width, int height, unsigned char* buff, int new);
void texture_delete(struct texture* t);
void texture_draw(struct texture* t, float x, float y, float w, float h);
void texture_draw_sector(struct texture* t, float x, float y, float w, float h, float u, float v, float us, float vs);
void texture_draw_empty(float x, float y, float w, float h);
void texture_draw_empty_rotated(float x, float y, float w, float h, float angle);
void texture_draw_rotated(struct texture* t, float x, float y, float w, float h, float angle);
void texture_resize_pow2(struct texture* t, int min_size);
unsigned int texture_block_color(int x, int y);
void texture_gradient_fog(unsigned int* gradient);

#endif
