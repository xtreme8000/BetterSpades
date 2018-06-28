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

#define BETTERSPADES_VERSION    "v0.1.2"

#define BETTERSPADES_PATCH      2
#define BETTERSPADES_MINOR      1
#define BETTERSPADES_MAJOR      0

#if __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include "GL/glext.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <enet/enet.h>

#include "GLFW/glfw3.h"
#include "lodepng/lodepng.h"
#include "libdeflate.h"
#include "ini.h"

#ifdef _WIN32
#define OS_WINDOWS
#endif

#ifdef __linux__
#define OS_LINUX
#endif

#ifdef __APPLE__
#define OS_APPLE
#endif

#ifdef DEVICE_RASPBERRYPI
#define OS_LINUX
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#define absf(a) (((a)>0)?(a):-(a))

#define distance2D(x1,y1,x2,y2) (((x2)-(x1))*((x2)-(x1))+((y2)-(y1))*((y2)-(y1)))
#define distance3D(x1,y1,z1,x2,y2,z2) (((x2)-(x1))*((x2)-(x1))+((y2)-(y1))*((y2)-(y1))+((z2)-(z1))*((z2)-(z1)))

#define rgb(r,g,b)	(((b)<<16)|((g)<<8)|(r))
#define red(col)	((col)&0xFF)
#define green(col)	(((col)>>8)&0xFF)
#define blue(col)	(((col)>>16)&0xFF)
#define alpha(col)	(((col)>>24)&0xFF)

#define PI			3.1415F
#define DOUBLEPI	(PI*2.0F)
#define HALFPI		(PI*0.5F)
#define EPSILON		0.005F

#define MOUSE_SENSITIVITY 0.002F

#include "glx.h"
#include "list.h"
#include "window.h"
#include "sound.h"
#include "texture.h"
#include "chunk.h"
#include "map.h"
#include "aabb.h"
#include "model.h"
#include "file.h"
#include "camera.h"
#include "network.h"
#include "player.h"
#include "particle.h"
#include "grenade.h"
#include "cameracontroller.h"
#include "font.h"
#include "weapon.h"
#include "matrix.h"
#include "tracer.h"
#include "config.h"
#include "hud.h"

void reshape(struct window_instance* window, int width, int height);
void text_input(struct window_instance* window, unsigned int codepoint);
void keys(struct window_instance* window, int key, int scancode, int action, int mods);
void mouse_click(struct window_instance* window, int button, int action, int mods);
void mouse(struct window_instance* window, double x, double y);
void mouse_scroll(struct window_instance* window, double xoffset, double yoffset);
void on_error(int i, const char* s);

unsigned char key_map[512];
unsigned char button_map[3];
unsigned char draw_outline;

#define CHAT_NO_INPUT	0
#define CHAT_ALL_INPUT	1
#define CHAT_TEAM_INPUT	2

extern int chat_input_mode;
extern float last_cy;

extern int fps;

extern char chat[2][10][256];
extern unsigned int chat_color[2][10];
extern float chat_timer[2][10];
extern char chat_popup[256];
extern float chat_popup_timer;
extern float chat_popup_duration;
void chat_add(int channel, unsigned int color, const char* msg);
void chat_showpopup(const char* msg, float duration);
const char* reason_disconnect(int code);

#define SCREEN_NONE			0
#define SCREEN_TEAM_SELECT	1
#define SCREEN_GUN_SELECT	2

extern char text_input_first;

extern int ms_seed;
int ms_rand(void);

#define CHECK_ALLOCATION_ERROR(ret) if (!ret) { \
printf("Critical error: memory allocation failed (%s:%d)", __func__, __LINE__); \
exit(1); \
}
