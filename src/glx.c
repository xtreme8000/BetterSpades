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

#include "common.h"

//for future opengl-es abstraction layer

int glx_version = 0;

int glx_fog = 0;

PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;

static int glx_major_ver() {
	return atoi(glGetString(GL_VERSION));
}

void glx_init() {
	if(glx_major_ver()>=2) {
		#ifdef OS_WINDOWS
			glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
			glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
			glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		#endif
		#if defined(OS_LINUX) || defined(OS_APPLE)
			glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress("glGenBuffers");
			glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress("glBindBuffer");
			glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress("glBufferData");
		#endif
		glx_version = 1;
	} else {
		glx_version = 0;
	}
}

int glx_displaylist_create() {
	int ret;
	if(!glx_version)
		ret = glGenLists(1);
	else
		glGenBuffers(2,&ret);
	return ret;
}

void glx_displaylist_update(int id, int size, unsigned char* color, short* vertex) {
	if(!glx_version) {
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);

		glNewList(id,GL_COMPILE);
		if(size>0) {
			glColorPointer(3,GL_UNSIGNED_BYTE,0,color);
			glVertexPointer(3,GL_SHORT,0,vertex);
			glDrawArrays(GL_QUADS,0,size);
		}
		glEndList();

		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	} else {
		glBindBuffer(GL_ARRAY_BUFFER,id);
		glBufferData(GL_ARRAY_BUFFER,size*3*2,vertex,GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER,id+1);
		glBufferData(GL_ARRAY_BUFFER,size*3,color,GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER,0);
	}
}

void glx_displaylist_draw(int id, int size) {
	if(!glx_version) {
		glCallList(id);
	} else {
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER,id);
		glVertexPointer(3,GL_SHORT,0,NULL);
		glBindBuffer(GL_ARRAY_BUFFER,id+1);
		glColorPointer(3,GL_UNSIGNED_BYTE,0,NULL);
		glDrawArrays(GL_QUADS,0,size);
		glBindBuffer(GL_ARRAY_BUFFER,0);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);

	}
}

void glx_enable_sphericalfog() {
	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);
	glBindTexture(GL_TEXTURE_2D,texture_gradient.texture_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
	float t_plane[4] = {1.0F/settings.render_distance/2.0F,0.0F,0.0F,-camera_x/settings.render_distance/2.0F+0.5F};
	float s_plane[4] = {0.0F,0.0F,1.0F/settings.render_distance/2.0F,-camera_z/settings.render_distance/2.0F+0.5F};
	glTexGenfv(GL_T,GL_EYE_PLANE,t_plane);
	glTexGenfv(GL_S,GL_EYE_PLANE,s_plane);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_S);
	glx_fog = 1;
}

void glx_disable_sphericalfog() {
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_S);
	glBindTexture(GL_TEXTURE_2D,0);
	glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glDisable(GL_TEXTURE_2D);
	glx_fog = 0;
}
