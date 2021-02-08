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

#ifndef TESSELATOR_H
#define TESSELATOR_H

#include <stdint.h>

#include "glx.h"

#ifdef OPENGL_ES
#define TESSELATE_TRIANGLES
#else
#define TESSELATE_QUADS
#endif

enum tesselator_vertex_type {
	VERTEX_INT,
	VERTEX_FLOAT,
};

struct tesselator {
	void* vertices;
	int8_t* normals;
	uint32_t* colors;
	uint32_t quad_count;
	uint32_t quad_space;
	int has_normal;
	uint32_t color;
	int8_t normal[3];
	enum tesselator_vertex_type vertex_type;
};

enum tesselator_cube_face {
	CUBE_FACE_X_N,
	CUBE_FACE_X_P,
	CUBE_FACE_Y_N,
	CUBE_FACE_Y_P,
	CUBE_FACE_Z_N,
	CUBE_FACE_Z_P,
};

void tesselator_create(struct tesselator* t, enum tesselator_vertex_type type, int has_normal);
void tesselator_clear(struct tesselator* t);
void tesselator_free(struct tesselator* t);
void tesselator_draw(struct tesselator* t, int with_color);
void tesselator_glx(struct tesselator* t, struct glx_displaylist* x);
void tesselator_set_color(struct tesselator* t, uint32_t color);
void tesselator_set_normal(struct tesselator* t, int8_t x, int8_t y, int8_t z);
void tesselator_addi(struct tesselator* t, int16_t* coords, uint32_t* colors, int8_t* normals);
void tesselator_addf(struct tesselator* t, float* coords, uint32_t* colors, int8_t* normals);
void tesselator_addi_simple(struct tesselator* t, int16_t* coords);
void tesselator_addf_simple(struct tesselator* t, float* coords);
void tesselator_addi_cube_face(struct tesselator* t, enum tesselator_cube_face face, int16_t x, int16_t y, int16_t z);
void tesselator_addi_cube_face_adv(struct tesselator* t, enum tesselator_cube_face face, int16_t x, int16_t y,
								   int16_t z, int16_t sx, int16_t sy, int16_t sz);
void tesselator_addf_cube_face(struct tesselator* t, enum tesselator_cube_face face, float x, float y, float z,
							   float sz);

#endif
