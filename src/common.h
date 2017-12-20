#include <GL/gl.h>
#include <GL/glu.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "GL/glext.h"

#include "GLFW/glfw3.h"

#include "lodepng/lodepng.h"

#include <enet/enet.h>
#include "libdeflate.h"

#include "ini.h"

#ifdef _WIN32
#define OS_WINDOWS
#endif

#ifdef __linux__
#define OS_LINUX
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

typedef unsigned char boolean;
#define true 1
#define false 0

#define rgb(r,g,b)	(((b)<<16)|((g)<<8)|(r))
#define red(col)	((col)&0xFF)
#define green(col)	(((col)>>8)&0xFF)
#define blue(col)	(((col)>>16)&0xFF)


#define PI			3.1415F
#define DOUBLEPI	(PI*2.0F)
#define HALFPI		(PI*0.5F)
#define EPSILON		0.005F

#define MOUSE_SENSITIVITY 0.002F

#include "glx.h"

#include "sound.h"
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
#include "texture.h"
#include "weapon.h"
#include "matrix.h"
#include "tracer.h"
#include "config.h"

unsigned char key_map[512];
unsigned char button_map[3];
unsigned char draw_outline;

/*PFNGLPOINTPARAMETERFVPROC glPointParameterfv;
PFNGLPOINTPARAMETERFPROC glPointParameterf;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLTEXIMAGE3DPROC glTexImage3D;*/

#define CHAT_NO_INPUT	0
#define CHAT_ALL_INPUT	1
#define CHAT_TEAM_INPUT	2

extern int chat_input_mode;
extern float last_cy;

extern char chat[2][10][256];
extern unsigned int chat_color[2][10];
extern float chat_timer[2][10];
extern char chat_popup[256];
extern float chat_popup_timer;
void chat_add(int channel, unsigned int color, const char* msg);
void chat_showpopup(const char* msg);

#define team_execute(t,team1,team2) switch(t) { case TEAM_1:{team1; break;} case TEAM_2:{team2; break;} }
#define team_setcolor(t) team_execute(t,glColor3ub(gamestate.gamemode.team_1.red,gamestate.gamemode.team_1.green,gamestate.gamemode.team_1.blue),glColor3ub(gamestate.gamemode.team_2.red,gamestate.gamemode.team_2.green,gamestate.gamemode.team_2.blue))

void glxcheckErrors(char* file, int line);

#define SCREEN_NONE			0
#define SCREEN_TEAM_SELECT	1
#define SCREEN_GUN_SELECT	2

extern int screen_current;
