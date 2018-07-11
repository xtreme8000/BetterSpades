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

//for future opengl es abstraction layer

extern int glx_version;
extern int glx_fog;

#ifndef OPENGL_ES
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;

extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLUSEPROGRAMPROC glUseProgram;

extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
#endif

struct glx_displaylist {
	int legacy;
	int modern[3];
	int size;
	int has_normal;
};

enum {
    GLX_DISPLAYLIST_NORMAL,
    GLX_DISPLAYLIST_ENHANCED
};

void glx_init(void);

int glx_vertex_shader(const char* vertex, const char* fragment);

void glx_enable_sphericalfog(void);
void glx_disable_sphericalfog(void);

void glx_displaylist_create(struct glx_displaylist* x);
void glx_displaylist_destroy(struct glx_displaylist* x);
void glx_displaylist_update(struct glx_displaylist* x, int size, int type, void* color, void* vertex, void* normal);
void glx_displaylist_draw(struct glx_displaylist* x, int type);
