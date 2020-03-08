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

extern float* matrix_current;

extern float matrix_view[16];
extern float matrix_model[16];
extern float matrix_projection[16];

#define MATRIX_STACK_DEPTH 8
extern float matrix_stack[MATRIX_STACK_DEPTH][16];
extern int matrix_stack_index;

void matrix_select(float* m);
void matrix_multiply(float* m);
void matrix_load(float* m);
void matrix_rotate(float angle, float x, float y, float z);
void matrix_translate(float x, float y, float z);
void matrix_scale3(float s);
void matrix_scale(float sx, float sy, float sz);
void matrix_identity(void);
void matrix_push(void);
void matrix_pop(void);
void matrix_vector(float* v);
void matrix_ortho(float left, float right, float bottom, float top, float nearv, float farv);
void matrix_perspective(float fovy, float aspect, float zNear, float zFar);
void matrix_lookAt(double eyex, double eyey, double eyez, double centerx, double centery, double centerz, double upx,
				   double upy, double upz);
void matrix_upload(void);
void matrix_upload_p(void);
void matrix_pointAt(float dx, float dy, float dz);

#endif
