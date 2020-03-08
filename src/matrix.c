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

float* matrix_current = matrix_view;
float matrix_view[16];
float matrix_model[16];
float matrix_projection[16];

float matrix_stack[MATRIX_STACK_DEPTH][16];
int matrix_stack_index = 0;

void matrix_select(float* m) {
	matrix_current = m;
}

void matrix_multiply(float* a) {
	float m[16];
	memcpy(m, matrix_current, 16 * sizeof(float));

	matrix_current[0] = a[0] * m[0] + a[4] * m[1] + a[8] * m[2] + a[12] * m[3];
	matrix_current[1] = a[1] * m[0] + a[5] * m[1] + a[9] * m[2] + a[13] * m[3];
	matrix_current[2] = a[2] * m[0] + a[6] * m[1] + a[10] * m[2] + a[14] * m[3];
	matrix_current[3] = a[3] * m[0] + a[7] * m[1] + a[11] * m[2] + a[15] * m[3];

	matrix_current[4] = a[0] * m[4] + a[4] * m[5] + a[8] * m[6] + a[12] * m[7];
	matrix_current[5] = a[1] * m[4] + a[5] * m[5] + a[9] * m[6] + a[13] * m[7];
	matrix_current[6] = a[2] * m[4] + a[6] * m[5] + a[10] * m[6] + a[14] * m[7];
	matrix_current[7] = a[3] * m[4] + a[7] * m[5] + a[11] * m[6] + a[15] * m[7];

	matrix_current[8] = a[0] * m[8] + a[4] * m[9] + a[8] * m[10] + a[12] * m[11];
	matrix_current[9] = a[1] * m[8] + a[5] * m[9] + a[9] * m[10] + a[13] * m[11];
	matrix_current[10] = a[2] * m[8] + a[6] * m[9] + a[10] * m[10] + a[14] * m[11];
	matrix_current[11] = a[3] * m[8] + a[7] * m[9] + a[11] * m[10] + a[15] * m[11];

	matrix_current[12] = a[0] * m[12] + a[4] * m[13] + a[8] * m[14] + a[12] * m[15];
	matrix_current[13] = a[1] * m[12] + a[5] * m[13] + a[9] * m[14] + a[13] * m[15];
	matrix_current[14] = a[2] * m[12] + a[6] * m[13] + a[10] * m[14] + a[14] * m[15];
	matrix_current[15] = a[3] * m[12] + a[7] * m[13] + a[11] * m[14] + a[15] * m[15];
}

void matrix_load(float* m) {
	memcpy(matrix_current, m, 16 * sizeof(float));
}

void matrix_rotate(float angle, float x, float y, float z) {
	float len = sqrt(x * x + y * y + z * z);
	if(len == 0.0F) {
		return;
	}

	if(len != 1.0) {
		x /= len;
		y /= len;
		z /= len;
	}

	float c = cos(angle / 180.0F * 3.1415926535F);
	float s = sin(angle / 180.0F * 3.1415926535F);
	float c2 = 1.0F - c;
	float a[16];
	memcpy(a, matrix_current, 16 * sizeof(float));

	matrix_current[0] = a[0] * (x * x * c2 + c) + a[4] * (y * x * c2 + z * s) + a[8] * (z * x * c2 - y * s);
	matrix_current[1] = a[1] * (x * x * c2 + c) + a[5] * (y * x * c2 + z * s) + a[9] * (z * x * c2 - y * s);
	matrix_current[2] = a[2] * (x * x * c2 + c) + a[6] * (y * x * c2 + z * s) + a[10] * (z * x * c2 - y * s);
	matrix_current[3] = a[3] * (x * x * c2 + c) + a[7] * (y * x * c2 + z * s) + a[11] * (z * x * c2 - y * s);

	matrix_current[4] = a[0] * (x * y * c2 - z * s) + a[4] * (y * y * c2 + c) + a[8] * (z * y * c2 + x * s);
	matrix_current[5] = a[1] * (x * y * c2 - z * s) + a[5] * (y * y * c2 + c) + a[9] * (z * y * c2 + x * s);
	matrix_current[6] = a[2] * (x * y * c2 - z * s) + a[6] * (y * y * c2 + c) + a[10] * (z * y * c2 + x * s);
	matrix_current[7] = a[3] * (x * y * c2 - z * s) + a[7] * (y * y * c2 + c) + a[11] * (z * y * c2 + x * s);

	matrix_current[8] = a[0] * (x * z * c2 + y * s) + a[4] * (y * z * c2 - x * s) + a[8] * (z * z * c2 + c);
	matrix_current[9] = a[1] * (x * z * c2 + y * s) + a[5] * (y * z * c2 - x * s) + a[9] * (z * z * c2 + c);
	matrix_current[10] = a[2] * (x * z * c2 + y * s) + a[6] * (y * z * c2 - x * s) + a[10] * (z * z * c2 + c);
	matrix_current[11] = a[3] * (x * z * c2 + y * s) + a[7] * (y * z * c2 - x * s) + a[11] * (z * z * c2 + c);
}

void matrix_translate(float x, float y, float z) {
	matrix_current[12] += matrix_current[0] * x + matrix_current[4] * y + matrix_current[8] * z;
	matrix_current[13] += matrix_current[1] * x + matrix_current[5] * y + matrix_current[9] * z;
	matrix_current[14] += matrix_current[2] * x + matrix_current[6] * y + matrix_current[10] * z;
	matrix_current[15] += matrix_current[3] * x + matrix_current[7] * y + matrix_current[11] * z;
}

void matrix_scale3(float s) {
	matrix_scale(s, s, s);
}

void matrix_scale(float sx, float sy, float sz) {
	matrix_current[0] *= sx;
	matrix_current[1] *= sx;
	matrix_current[2] *= sx;
	matrix_current[3] *= sx;

	matrix_current[4] *= sy;
	matrix_current[5] *= sy;
	matrix_current[6] *= sy;
	matrix_current[7] *= sy;

	matrix_current[8] *= sz;
	matrix_current[9] *= sz;
	matrix_current[10] *= sz;
	matrix_current[11] *= sz;
}

void matrix_identity() {
	for(int k = 0; k < 16; k++)
		matrix_current[k] = (k % 4) == (k / 4);
}

void matrix_push() {
	if(matrix_stack_index >= MATRIX_STACK_DEPTH) {
		log_fatal("Matrix stack overflow!");
		return;
	}
	memcpy(matrix_stack[matrix_stack_index++], matrix_current, 16 * sizeof(float));
}

void matrix_pop() {
	if(matrix_stack_index < 1) {
		log_fatal("Matrix stack underflow!");
		return;
	}
	memcpy(matrix_current, matrix_stack[--matrix_stack_index], 16 * sizeof(float));
}

void matrix_vector(float* v) {
	float tmp[4]
		= {v[0] * matrix_current[0] + v[1] * matrix_current[4] + v[2] * matrix_current[8] + v[3] * matrix_current[12],
		   v[0] * matrix_current[1] + v[1] * matrix_current[5] + v[2] * matrix_current[9] + v[3] * matrix_current[13],
		   v[0] * matrix_current[2] + v[1] * matrix_current[6] + v[2] * matrix_current[10] + v[3] * matrix_current[14],
		   v[0] * matrix_current[3] + v[1] * matrix_current[7] + v[2] * matrix_current[11] + v[3] * matrix_current[15]};
	tmp[0] /= tmp[3];
	tmp[1] /= tmp[3];
	tmp[2] /= tmp[3];
	memcpy(v, tmp, 4 * sizeof(float));
}

void matrix_pointAt(float dx, float dy, float dz) {
	float l = sqrt(dx * dx + dy * dy + dz * dz);
	if(l) {
		dx /= l;
		dy /= l;
		dz /= l;
	}
	float rx = -atan2(dz, dx) / 3.1415926535F * 180.0F;
	matrix_rotate(rx, 0.0F, 1.0F, 0.0F);
	if(dy) {
		float ry = asin(dy) / 3.1415926535F * 180.0F;
		matrix_rotate(ry, 0.0F, 0.0F, 1.0F);
	}
}

void matrix_ortho(float left, float right, float bottom, float top, float nearv, float farv) {
	float a = right - left;
	float b = top - bottom;
	float c = farv - nearv;

	float d = -(right + left) / a;
	float e = -(top + bottom) / b;
	float f = -(farv + nearv) / c;

	matrix_current[12] += matrix_current[0] * d + matrix_current[4] * e + matrix_current[8] * f;
	matrix_current[13] += matrix_current[1] * d + matrix_current[5] * e + matrix_current[9] * f;
	matrix_current[14] += matrix_current[2] * d + matrix_current[6] * e + matrix_current[10] * f;
	matrix_current[15] += matrix_current[3] * d + matrix_current[7] * e + matrix_current[11] * f;
	d = 2.0F / a;
	matrix_current[0] *= d;
	matrix_current[1] *= d;
	matrix_current[2] *= d;
	matrix_current[3] *= d;
	d = 2.0F / b;
	matrix_current[4] *= d;
	matrix_current[5] *= d;
	matrix_current[6] *= d;
	matrix_current[7] *= d;
	d = -2.0F / c;
	matrix_current[8] *= d;
	matrix_current[9] *= d;
	matrix_current[10] *= d;
	matrix_current[11] *= d;
}

void matrix_perspective(float fovy, float aspect, float zNear, float zFar) {
	float f = 1.0F / tan(fovy * 0.5F / 180.0F * 3.1415926535F);

	float tmp[4] = {matrix_current[8], matrix_current[9], matrix_current[10], matrix_current[11]};

	matrix_current[0] *= (f / aspect);
	matrix_current[1] *= (f / aspect);
	matrix_current[2] *= (f / aspect);
	matrix_current[3] *= (f / aspect);

	matrix_current[4] *= f;
	matrix_current[5] *= f;
	matrix_current[6] *= f;
	matrix_current[7] *= f;

	matrix_current[8] = tmp[0] * ((zFar + zNear) / (zNear - zFar)) - matrix_current[12];
	matrix_current[9] = tmp[1] * ((zFar + zNear) / (zNear - zFar)) - matrix_current[13];
	matrix_current[10] = tmp[2] * ((zFar + zNear) / (zNear - zFar)) - matrix_current[14];
	matrix_current[11] = tmp[3] * ((zFar + zNear) / (zNear - zFar)) - matrix_current[15];

	matrix_current[12] = tmp[0] * ((2.0F * zFar * zNear) / (zNear - zFar));
	matrix_current[13] = tmp[1] * ((2.0F * zFar * zNear) / (zNear - zFar));
	matrix_current[14] = tmp[2] * ((2.0F * zFar * zNear) / (zNear - zFar));
	matrix_current[15] = tmp[3] * ((2.0F * zFar * zNear) / (zNear - zFar));
}

void matrix_lookAt(double eyex, double eyey, double eyez, double centerx, double centery, double centerz, double upx,
				   double upy, double upz) {
	double fx = centerx - eyex;
	double fy = centery - eyey;
	double fz = centerz - eyez;

	double l = sqrt(fx * fx + fy * fy + fz * fz);
	if(l) {
		fx /= l;
		fy /= l;
		fz /= l;
	}

	l = sqrt(upx * upx + upy * upy + upz * upz);
	if(l) {
		upx /= l;
		upy /= l;
		upz /= l;
	}

	double sx = fy * upz - upy * fz;
	double sy = fz * upx - upz * fx;
	double sz = fx * upy - upx * fy;
	l = sqrt(sx * sx + sy * sy + sz * sz);
	if(l) {
		sx /= l;
		sy /= l;
		sz /= l;
	}

	double ux = sy * fz - fy * sz;
	double uy = sz * fx - fz * sx;
	double uz = sx * fy - fx * sy;
	l = sqrt(fx * fx + fy * fy + fz * fz);
	if(l) {
		fx /= l;
		fy /= l;
		fz /= l;
	}

	float a[12];
	memcpy(a, matrix_current, 12 * sizeof(float));

	matrix_current[0] = a[0] * sx + a[4] * ux - a[8] * fx;
	matrix_current[1] = a[1] * sx + a[5] * ux - a[9] * fx;
	matrix_current[2] = a[2] * sx + a[6] * ux - a[10] * fx;
	matrix_current[3] = a[3] * sx + a[7] * ux - a[11] * fx;

	matrix_current[4] = a[0] * sy + a[4] * uy - a[8] * fy;
	matrix_current[5] = a[1] * sy + a[5] * uy - a[9] * fy;
	matrix_current[6] = a[2] * sy + a[6] * uy - a[10] * fy;
	matrix_current[7] = a[3] * sy + a[7] * uy - a[11] * fy;

	matrix_current[8] = a[0] * sz + a[4] * uz - a[8] * fz;
	matrix_current[9] = a[1] * sz + a[5] * uz - a[9] * fz;
	matrix_current[10] = a[2] * sz + a[6] * uz - a[10] * fz;
	matrix_current[11] = a[3] * sz + a[7] * uz - a[11] * fz;

	matrix_translate(-eyex, -eyey, -eyez);
}

void matrix_upload() {
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(matrix_view);
	glMultMatrixf(matrix_model);
}

void matrix_upload_p() {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(matrix_projection);
}
