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

struct hud {
	void (*init) ();
	void (*render_3D) ();
	void (*render_2D) (float scalex, float scaley);
	void (*input_keyboard) (int key, int action, int mods, int internal);
	void (*input_mouselocation) (double x, double y);
	void (*input_mouseclick) (double x, double y, int button, int action, int mods);
	void (*input_mousescroll) (double yoffset);
	void (*input_touch) (void* finger, int action, float x, float y, float dx, float dy);
	char render_world;
	char render_localplayer;
};

struct serverlist_entry {
	int current, max;
	char name[32];
	char map[21];
	char gamemode[8];
	int ping;
	char identifier[32];
	char country[4];
};

extern int screen_current;

extern struct hud hud_ingame;
extern struct hud hud_mapload;
extern struct hud hud_serverlist;
extern struct hud hud_settings;
extern struct hud hud_controls;

extern struct hud* hud_active;
extern struct window_instance* hud_window;

void hud_change(struct hud* new);
void hud_init();
void hud_mousemode(int mode);
