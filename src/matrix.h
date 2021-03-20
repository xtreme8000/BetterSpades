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

#ifndef MATRIX_H
#define MATRIX_H

#include <cglm/call.h>

extern mat4 matrix_view;
extern mat4 matrix_model;
extern mat4 matrix_projection;

void matrix_multiply(mat4 m, mat4 n);
void matrix_load(mat4 m, mat4 n);
void matrix_rotate(mat4 m, float angle, float x, float y, float z);
void matrix_translate(mat4 m, float x, float y, float z);
void matrix_scale3(mat4 m, float s);
void matrix_scale(mat4 m, float sx, float sy, float sz);
void matrix_identity(mat4 m);
void matrix_push(mat4 m);
void matrix_pop(mat4 m);
void matrix_vector(mat4 m, vec4 v);
void matrix_ortho(mat4 m, float left, float right, float bottom, float top, float nearv, float farv);
void matrix_perspective(mat4 m, float fovy, float aspect, float zNear, float zFar);
void matrix_lookAt(mat4 m, double eyex, double eyey, double eyez, double centerx, double centery, double centerz,
				   double upx, double upy, double upz);
void matrix_upload(void);
void matrix_upload_p(void);
void matrix_pointAt(mat4 m, float dx, float dy, float dz);

#endif
