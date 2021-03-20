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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "matrix.h"

mat4 matrix_view;
mat4 matrix_model;
mat4 matrix_projection;

#define MATRIX_STACK_DEPTH 8

mat4 matrix_stack[MATRIX_STACK_DEPTH];
int matrix_stack_index = 0;

void matrix_multiply(mat4 m, mat4 n) {
	glmc_mat4_mul(n, m, m);
}

void matrix_load(mat4 m, mat4 n) {
	glmc_mat4_copy(n, m);
}

void matrix_rotate(mat4 m, float angle, float x, float y, float z) {
	glmc_rotate(m, angle / 180.0F * GLM_PI, (vec3) {x, y, z});
}

void matrix_translate(mat4 m, float x, float y, float z) {
	glmc_translate(m, (vec3) {x, y, z});
}

void matrix_scale3(mat4 m, float s) {
	glmc_scale_uni(m, s);
}

void matrix_scale(mat4 m, float sx, float sy, float sz) {
	glmc_scale(m, (vec3) {sx, sy, sz});
}

void matrix_identity(mat4 m) {
	glmc_mat4_identity(m);
}

void matrix_push(mat4 m) {
	if(matrix_stack_index >= MATRIX_STACK_DEPTH) {
		log_fatal("Matrix stack overflow!");
		return;
	}

	glmc_mat4_copy(m, matrix_stack[matrix_stack_index++]);
}

void matrix_pop(mat4 m) {
	if(matrix_stack_index < 1) {
		log_fatal("Matrix stack underflow!");
		return;
	}

	glmc_mat4_copy(matrix_stack[--matrix_stack_index], m);
}

void matrix_vector(mat4 m, vec4 v) {
	glmc_mat4_mulv(m, v, v);

	v[0] /= v[3];
	v[1] /= v[3];
	v[2] /= v[3];
}

void matrix_pointAt(mat4 m, float dx, float dy, float dz) {
	float l = sqrt(dx * dx + dy * dy + dz * dz);
	if(l) {
		dx /= l;
		dy /= l;
		dz /= l;
	}
	float rx = -atan2(dz, dx) * GLM_1_PI * 180.0F;
	matrix_rotate(m, rx, 0.0F, 1.0F, 0.0F);
	if(dy) {
		float ry = asin(dy) * GLM_1_PI * 180.0F;
		matrix_rotate(m, ry, 0.0F, 0.0F, 1.0F);
	}
}

void matrix_ortho(mat4 m, float left, float right, float bottom, float top, float nearv, float farv) {
	glmc_ortho(left, right, bottom, top, nearv, farv, m);
}

void matrix_perspective(mat4 m, float fovy, float aspect, float zNear, float zFar) {
	glmc_perspective(fovy / 180.0F * GLM_PI, aspect, zNear, zFar, m);
}

void matrix_lookAt(mat4 m, double eyex, double eyey, double eyez, double centerx, double centery, double centerz,
				   double upx, double upy, double upz) {
	glmc_lookat((vec3) {eyex, eyey, eyez}, (vec3) {centerx, centery, centerz}, (vec3) {upx, upy, upz}, m);
	// matrix_translate(-eyex, -eyey, -eyez);
}

void matrix_upload() {
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float*)matrix_view);
	glMultMatrixf((float*)matrix_model);
}

void matrix_upload_p() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)matrix_projection);
}
