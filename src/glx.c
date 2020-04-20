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

#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "camera.h"
#include "config.h"
#include "map.h"
#include "matrix.h"
#include "texture.h"
#include "glx.h"

// for future opengl-es abstraction layer

int glx_version = 0;

int glx_fog = 0;

static int glx_major_ver() {
#ifdef OPENGL_ES
	return 2;
#else
	return atoi(glGetString(GL_VERSION));
#endif
}

void glx_init() {
#ifndef OPENGL_ES
	glx_version = glx_major_ver() >= 2;
#else
	glx_version = 0;
#endif
}

int glx_shader(const char* vertex, const char* fragment) {
#ifndef OPENGL_ES
	int v, f;
	if(vertex) {
		v = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(v, 1, (const GLchar* const*)&vertex, NULL);
		glCompileShader(v);
	}

	if(fragment) {
		f = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(f, 1, (const GLchar* const*)&fragment, NULL);
		glCompileShader(f);
	}

	int program = glCreateProgram();
	if(vertex)
		glAttachShader(program, v);
	if(vertex)
		glAttachShader(program, f);
	glLinkProgram(program);
	return program;
#else
	return 0;
#endif
}

void glx_displaylist_create(struct glx_displaylist* x, bool has_color, bool has_normal) {
	x->has_color = has_color;
	x->has_normal = has_normal;

#ifndef OPENGL_ES
	if(!glx_version || settings.force_displaylist) {
		x->legacy = glGenLists(1);
	} else {
		glGenBuffers(1, x->modern + 0);

		if(has_color)
			glGenBuffers(1, x->modern + 1);
		if(has_normal)
			glGenBuffers(1, x->modern + 2);
	}
#else
	glGenBuffers(1, x->modern + 0);

	if(has_color)
		glGenBuffers(1, x->modern + 1);
	if(has_normal)
		glGenBuffers(1, x->modern + 2);
#endif
	x->buffer_size = 0;
}

void glx_displaylist_destroy(struct glx_displaylist* x) {
#ifndef OPENGL_ES
	if(!glx_version || settings.force_displaylist) {
		glDeleteLists(x->legacy, 1);
	} else {
		glDeleteBuffers(1, x->modern + 0);

		if(x->has_color)
			glDeleteBuffers(1, x->modern + 1);
		if(x->has_normal)
			glDeleteBuffers(1, x->modern + 2);
	}
#else
	glDeleteBuffers(1, x->modern + 0);

	if(x->has_color)
		glDeleteBuffers(1, x->modern + 1);
	if(x->has_normal)
		glDeleteBuffers(1, x->modern + 2);
#endif
}

void glx_displaylist_update(struct glx_displaylist* x, size_t size, int type, void* color, void* vertex, void* normal) {
	int grow_buffer = size > x->buffer_size;
	x->buffer_size = max(x->buffer_size, size);
	x->size = size;
#ifndef OPENGL_ES
	if(!glx_version || settings.force_displaylist) {
		glEnableClientState(GL_VERTEX_ARRAY);
		if(x->has_color)
			glEnableClientState(GL_COLOR_ARRAY);
		if(x->has_normal)
			glEnableClientState(GL_NORMAL_ARRAY);

		glNewList(x->legacy, GL_COMPILE);
		if(size > 0) {
			if(x->has_color)
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, color);

			switch(type) {
				case GLX_DISPLAYLIST_NORMAL: glVertexPointer(3, GL_SHORT, 0, vertex); break;
				case GLX_DISPLAYLIST_POINTS:
				case GLX_DISPLAYLIST_ENHANCED: glVertexPointer(3, GL_FLOAT, 0, vertex); break;
			}

			if(x->has_normal)
				glNormalPointer(GL_BYTE, 0, normal);
			glDrawArrays((type == GLX_DISPLAYLIST_POINTS) ? GL_POINTS : GL_QUADS, 0, x->size);
		}
		glEndList();

		glDisableClientState(GL_VERTEX_ARRAY);
		if(x->has_color)
			glDisableClientState(GL_COLOR_ARRAY);
		if(x->has_normal)
			glDisableClientState(GL_NORMAL_ARRAY);
	} else {
#endif
		glBindBuffer(GL_ARRAY_BUFFER, x->modern[0]);
		switch(type) {
			case GLX_DISPLAYLIST_NORMAL:
				if(grow_buffer) {
					glBufferData(GL_ARRAY_BUFFER, x->size * 3 * sizeof(short), vertex, GL_DYNAMIC_DRAW);
				} else {
					glBufferSubData(GL_ARRAY_BUFFER, 0, x->size * 3 * sizeof(short), vertex);
				}

				if(x->has_color) {
					glBindBuffer(GL_ARRAY_BUFFER, x->modern[1]);
					if(grow_buffer) {
						glBufferData(GL_ARRAY_BUFFER, x->size * 4, color, GL_DYNAMIC_DRAW);
					} else {
						glBufferSubData(GL_ARRAY_BUFFER, 0, x->size * 4, color);
					}
				}
				break;
			case GLX_DISPLAYLIST_POINTS:
			case GLX_DISPLAYLIST_ENHANCED:
				if(grow_buffer) {
					glBufferData(GL_ARRAY_BUFFER, x->size * 3 * sizeof(float), vertex, GL_DYNAMIC_DRAW);
				} else {
					glBufferSubData(GL_ARRAY_BUFFER, 0, x->size * 3 * sizeof(float), vertex);
				}

				if(x->has_color) {
					glBindBuffer(GL_ARRAY_BUFFER, x->modern[1]);
					if(grow_buffer) {
						glBufferData(GL_ARRAY_BUFFER, x->size * 4, color, GL_DYNAMIC_DRAW);
					} else {
						glBufferSubData(GL_ARRAY_BUFFER, 0, x->size * 4, color);
					}
				}
				break;
		}
		if(x->has_normal) {
			glBindBuffer(GL_ARRAY_BUFFER, x->modern[2]);
			if(grow_buffer) {
				glBufferData(GL_ARRAY_BUFFER, x->size * 3, normal, GL_DYNAMIC_DRAW);
			} else {
				glBufferSubData(GL_ARRAY_BUFFER, 0, x->size * 3, normal);
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
#ifndef OPENGL_ES
	}
#endif
}

void glx_displaylist_draw(struct glx_displaylist* x, int type) {
#ifndef OPENGL_ES
	if(!glx_version || settings.force_displaylist) {
		glCallList(x->legacy);
	} else {
#endif
		glEnableClientState(GL_VERTEX_ARRAY);

		if(x->has_color) {
			glEnableClientState(GL_COLOR_ARRAY);
			glBindBuffer(GL_ARRAY_BUFFER, x->modern[1]);
			glColorPointer(4, GL_UNSIGNED_BYTE, 0, NULL);
		}

		if(x->has_normal) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glBindBuffer(GL_ARRAY_BUFFER, x->modern[2]);
			glNormalPointer(GL_BYTE, 0, NULL);
		}

		glBindBuffer(GL_ARRAY_BUFFER, x->modern[0]);
		switch(type) {
			case GLX_DISPLAYLIST_NORMAL: glVertexPointer(3, GL_SHORT, 0, NULL); break;
			case GLX_DISPLAYLIST_POINTS:
			case GLX_DISPLAYLIST_ENHANCED: glVertexPointer(3, GL_FLOAT, 0, NULL); break;
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if(type == GLX_DISPLAYLIST_POINTS) {
			glDrawArrays(GL_POINTS, 0, x->size);
		} else {
#ifdef OPENGL_ES
			glDrawArrays(GL_TRIANGLES, 0, x->size);
#else
		glDrawArrays(GL_QUADS, 0, x->size);
#endif
		}

		if(x->has_normal)
			glDisableClientState(GL_NORMAL_ARRAY);
		if(x->has_color)
			glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
#ifndef OPENGL_ES
	}
#endif
}

void glx_enable_sphericalfog() {
#ifndef OPENGL_ES
	if(!settings.smooth_fog) {
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glBindTexture(GL_TEXTURE_2D, texture_gradient.texture_id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		float t_plane[4]
			= {1.0F / settings.render_distance / 2.0F, 0.0F, 0.0F, -camera_x / settings.render_distance / 2.0F + 0.5F};
		float s_plane[4]
			= {0.0F, 0.0F, 1.0F / settings.render_distance / 2.0F, -camera_z / settings.render_distance / 2.0F + 0.5F};
		glTexGenfv(GL_T, GL_EYE_PLANE, t_plane);
		glTexGenfv(GL_S, GL_EYE_PLANE, s_plane);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_S);
	} else {
		matrix_select(matrix_model);
		matrix_push();
		matrix_identity();
		matrix_upload();
		matrix_pop();

		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT1);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT, GL_DIFFUSE);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, (float[]) {fog_color[0], fog_color[1], fog_color[2], 1.0F});

		glLightfv(GL_LIGHT1, GL_POSITION,
				  (float[]) {camera_x, (settings.render_distance * map_size_y) / 16.0F, camera_z, 1.0F});
		float dir[3] = {0.0F, -1.0F, 0.0F};
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, (float[]) {0.0F, -1.0F, 0.0F});
		glLightfv(GL_LIGHT1, GL_DIFFUSE, (float[]) {1.0F, 1.0F, 1.0F, 1.0F});
		glLightfv(GL_LIGHT1, GL_AMBIENT, (float[]) {-fog_color[0], -fog_color[1], -fog_color[2], 1.0F});
		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, tan(16.0F / map_size_y) / PI * 180.0F);
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 128.0F);
		glNormal3f(0.0F, 1.0F, 0.0F);
	}
#else
	matrix_select(matrix_model);
	matrix_push();
	matrix_identity();
	matrix_upload();
	matrix_pop();

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT1);
	glEnable(GL_COLOR_MATERIAL);
	// glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
	float amb[4] = {0.0F, 0.0F, 0.0F, 1.0F};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

	float lpos[4] = {camera_x, (settings.render_distance * map_size_y) / 16.0F, camera_z, 1.0F};
	glLightfv(GL_LIGHT1, GL_POSITION, lpos);
	float dir[3] = {0.0F, -1.0F, 0.0F};
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dir);
	float dif[4] = {0.0F, 0.0F, 0.0F, 1.0F};
	glLightfv(GL_LIGHT1, GL_DIFFUSE, dif);
	float amb2[4] = {1.0F, 1.0F, 1.0F, 1.0F};
	glLightfv(GL_LIGHT1, GL_AMBIENT, amb2);
	glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, tan(16.0F / map_size_y) / PI * 180.0F);
	glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 128.0F);
	glNormal3f(0.0F, 1.0F, 0.0F);
	glEnable(GL_FOG);
	glFogf(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 0.0F);
	glFogf(GL_FOG_END, settings.render_distance);
	glFogfv(GL_FOG_COLOR, fog_color);
#endif
	glx_fog = 1;
}

void glx_disable_sphericalfog() {
#ifndef OPENGL_ES
	if(!settings.smooth_fog) {
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_S);
		glBindTexture(GL_TEXTURE_2D, 0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_2D);
	} else {
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHTING);
		float a[4] = {0.2F, 0.2F, 0.2F, 1.0F};
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, a);
	}
#else
	glDisable(GL_FOG);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);
	float a[4] = {0.2F, 0.2F, 0.2F, 1.0F};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, a);
#endif
	glx_fog = 0;
}
