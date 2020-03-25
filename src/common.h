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

#ifndef COMMON_H
#define COMMON_H

#ifndef OPENGL_ES
#define GLEW_STATIC
#include <GL/glew.h>
#else
#ifdef USE_SDL
#include <SDL2/SDL_opengles.h>
#endif
void glColor3f(float r, float g, float b);
void glColor3ub(unsigned char r, unsigned char g, unsigned char b);
void glDepthRange(float near, float far);
void glClearDepth(float x);
#endif

#ifdef USE_GLFW
#include <GLFW/glfw3.h>
#endif

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

#ifdef USE_RPC
#include <discord_rpc.h>
#endif

#ifdef _WIN32
#define OS_WINDOWS
#endif

#ifdef __linux__
#define OS_LINUX
#include <sys/sysinfo.h>
#endif

#ifdef __APPLE__
#define OS_APPLE
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#define absf(a) (((a) > 0) ? (a) : -(a))

#define distance2D(x1, y1, x2, y2) (((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1)))
#define distance3D(x1, y1, z1, x2, y2, z2)                                                                             \
	(((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1)) + ((z2) - (z1)) * ((z2) - (z1)))
#define angle3D(x1, y1, z1, x2, y2, z2) acos((x1) * (x2) + (y1) * (y2) + (z1) * (z2)) // vectors should be normalized
#define len2D(x, y) sqrt(pow(x, 2) + pow(y, 2))
#define len3D(x, y, z) sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2))

#define rgba(r, g, b, a) (((int)(a) << 24) | ((int)(b) << 16) | ((int)(g) << 8) | (int)(r))
#define rgb(r, g, b) (((b) << 16) | ((g) << 8) | (r))
#define rgb2bgr(col) rgb(blue(col), green(col), red(col))
#define red(col) ((col)&0xFF)
#define green(col) (((col) >> 8) & 0xFF)
#define blue(col) (((col) >> 16) & 0xFF)
#define alpha(col) (((col) >> 24) & 0xFF)

#define PI 3.1415F
#define DOUBLEPI (PI * 2.0F)
#define HALFPI (PI * 0.5F)
#define EPSILON 0.005F

#define MOUSE_SENSITIVITY 0.002F

#define CHAT_NO_INPUT 0
#define CHAT_ALL_INPUT 1
#define CHAT_TEAM_INPUT 2

extern int chat_input_mode;
extern float last_cy;

extern int fps;

extern char chat[2][10][256];
extern unsigned int chat_color[2][10];
extern float chat_timer[2][10];
extern char chat_popup[256];
extern float chat_popup_timer;
extern float chat_popup_duration;
extern int chat_popup_color;
void chat_add(int channel, unsigned int color, const char* msg);
void chat_showpopup(const char* msg, float duration, int color);
const char* reason_disconnect(int code);

#define SCREEN_NONE 0
#define SCREEN_TEAM_SELECT 1
#define SCREEN_GUN_SELECT 2

extern int ms_seed;
int ms_rand(void);

#include <stdlib.h>
#include "log.h"

#define CHECK_ALLOCATION_ERROR(ret)                                                                                    \
	if(!ret) {                                                                                                         \
		log_fatal("Critical error: memory allocation failed (%s:%d)", __func__, __LINE__);                             \
		exit(1);                                                                                                       \
	}

#endif
