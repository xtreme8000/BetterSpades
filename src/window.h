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


struct window_instance {
    void* impl;
};

enum {
    WINDOW_PRESS,
    WINDOW_RELEASE
};

enum window_keys {
	WINDOW_KEY_UNKNOWN,
    WINDOW_KEY_UP,
    WINDOW_KEY_DOWN,
    WINDOW_KEY_LEFT,
    WINDOW_KEY_RIGHT,
    WINDOW_KEY_SNEAK,
    WINDOW_KEY_CROUCH,
    WINDOW_KEY_SPRINT,
    WINDOW_KEY_SPACE,
    WINDOW_KEY_CURSOR_UP,
    WINDOW_KEY_CURSOR_DOWN,
    WINDOW_KEY_CURSOR_LEFT,
    WINDOW_KEY_CURSOR_RIGHT,
    WINDOW_KEY_TAB,
    WINDOW_KEY_ESCAPE,
    WINDOW_KEY_YES,
    WINDOW_KEY_NO,
    WINDOW_KEY_RELOAD,
    WINDOW_KEY_TOOL1,
    WINDOW_KEY_TOOL2,
    WINDOW_KEY_TOOL3,
    WINDOW_KEY_TOOL4,
    WINDOW_KEY_PICKCOLOR,
    WINDOW_KEY_CHAT,
    WINDOW_KEY_F1,
    WINDOW_KEY_F2,
    WINDOW_KEY_F3,
    WINDOW_KEY_F4,
    WINDOW_KEY_CHANGETEAM,
    WINDOW_KEY_CHANGEWEAPON,
    WINDOW_KEY_ENTER,
    WINDOW_KEY_BACKSPACE,
    WINDOW_KEY_MAP,
    WINDOW_KEY_VOLUME_UP,
    WINDOW_KEY_VOLUME_DOWN,
    WINDOW_KEY_V,
    WINDOW_KEY_FULLSCREEN,
    WINDOW_KEY_SCREENSHOT,
    WINDOW_KEY_COMMAND,
    WINDOW_KEY_HIDEHUD,
	WINDOW_KEY_LASTTOOL
};

enum {
    WINDOW_CURSOR_DISABLED,
    WINDOW_CURSOR_ENABLED
};

enum {
    WINDOW_MOUSE_LMB,
    WINDOW_MOUSE_MMB,
    WINDOW_MOUSE_RMB
};

extern int window_pressed_keys[64];

float window_time(void);
const char* window_clipboard(void);
int window_key_translate(int key, int dir);
int window_key_down(int key);
void window_mousemode(int mode);
void window_mouseloc(double* x, double* y);
void window_swapping(int value);
void window_init(void);
void window_fromsettings(void);
void window_deinit(void);
void window_update(void);
int window_closed(void);
