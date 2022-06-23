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
#include <string.h>
#include <math.h>

#include "common.h"
#include "file.h"
#include "hashtable.h"
#include "font.h"
#include "stb_truetype.h"
#include "utils.h"

#define FONT_BAKE_START 31

static short* font_vertex_buffer;
static short* font_coords_buffer;
static enum font_type font_current_type = FONT_FIXEDSYS;

static void* font_data_fixedsys;
static void* font_data_smallfnt;

static HashTable fonts_backed;

struct __attribute__((packed)) font_backed_id {
	enum font_type type;
	float size;
};

struct font_backed_data {
	stbtt_bakedchar* cdata;
	GLuint texture_id;
	int w, h;
};

void font_init() {
	font_vertex_buffer = malloc(512 * 8 * sizeof(short));
	CHECK_ALLOCATION_ERROR(font_vertex_buffer)
	font_coords_buffer = malloc(512 * 8 * sizeof(short));
	CHECK_ALLOCATION_ERROR(font_coords_buffer)

	font_data_fixedsys = file_load("fonts/Fixedsys.ttf");
	CHECK_ALLOCATION_ERROR(font_data_fixedsys)
	font_data_smallfnt = file_load("fonts/Terminal.ttf");
	CHECK_ALLOCATION_ERROR(font_data_smallfnt)

	ht_setup(&fonts_backed, sizeof(struct font_backed_id), sizeof(struct font_backed_data), 8);
}

void font_select(enum font_type type) {
	font_current_type = type;
}

static struct font_backed_data* font_find(float h) {
	if(font_current_type == FONT_SMALLFNT)
		h *= 1.5F;

	struct font_backed_id id = (struct font_backed_id) {
		.type = font_current_type,
		.size = h,
	};

	struct font_backed_data* f_cached = ht_lookup(&fonts_backed, &id);

	if(f_cached)
		return f_cached;

	void* file;
	switch(font_current_type) {
		case FONT_FIXEDSYS: file = font_data_fixedsys; break;
		case FONT_SMALLFNT: file = font_data_smallfnt; break;
		default: return NULL;
	}

	struct font_backed_data f;
	f.w = 64;
	f.h = 64;
	f.cdata = malloc((0xFF - FONT_BAKE_START) * sizeof(stbtt_bakedchar));
	CHECK_ALLOCATION_ERROR(f.cdata)

	void* temp_bitmap = NULL;

	int max_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);

	while(1) {
		temp_bitmap = realloc(temp_bitmap, f.w * f.h);
		CHECK_ALLOCATION_ERROR(temp_bitmap)
		int res = 0;
		res = stbtt_BakeFontBitmap(file, 0, h, temp_bitmap, f.w, f.h, FONT_BAKE_START, 0xFF - FONT_BAKE_START, f.cdata);
		if(res > 0 || (f.w == max_size && f.h == max_size))
			break;
		if(f.h > f.w)
			f.w *= 2;
		else
			f.h *= 2;
	}

	log_info("font texsize: %i:%ipx [size %f] type: %i", f.w, f.h, h, font_current_type);

	glGenTextures(1, &f.texture_id);
	glBindTexture(GL_TEXTURE_2D, f.texture_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, f.w, f.h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	free(temp_bitmap);

	ht_insert(&fonts_backed, &id, &f);

	return ht_lookup(&fonts_backed, &id);
}

float font_length(float h, char* text) {
	struct font_backed_data* font = font_find(h);

	if(!font)
		return 0.0F;

	stbtt_aligned_quad q;
	float y = h * 0.75F;
	float x = 0.0F;
	float length = 0.0F;
	for(size_t k = 0; k < strlen(text); k++) {
		if(text[k] == '\n') {
			length = fmax(length, x);
			x = 0.0F;
		}

		if(text[k] >= FONT_BAKE_START)
			stbtt_GetBakedQuad(font->cdata, font->w, font->h, text[k] - FONT_BAKE_START, &x, &y, &q, 1);
	}

	return fmax(length, x) + h * 0.125F;
}

bool font_remove_callback(void* key, void* value, void* user) {
	struct font_backed_data* f = (struct font_backed_data*)value;

	glDeleteTextures(1, &f->texture_id);
	free(f->cdata);

	return true;
}

void font_reset() {
	ht_iterate_remove(&fonts_backed, NULL, font_remove_callback);
}

void font_render(float x, float y, float h, char* text) {
	struct font_backed_data* font = font_find(h);

	if(!font)
		return;

	size_t k = 0;
	float x2 = x;
	float y2 = h * 0.75F;
	while(*text) {
		if(*text == '\n') {
			x2 = x;
			y2 += h;
		}

		if(*text >= FONT_BAKE_START) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(font->cdata, font->w, font->h, *text - FONT_BAKE_START, &x2, &y2, &q, 1);
			font_coords_buffer[k + 0] = q.s0 * 8192.0F;
			font_coords_buffer[k + 1] = q.t1 * 8192.0F;
			font_coords_buffer[k + 2] = q.s1 * 8192.0F;
			font_coords_buffer[k + 3] = q.t1 * 8192.0F;
			font_coords_buffer[k + 4] = q.s1 * 8192.0F;
			font_coords_buffer[k + 5] = q.t0 * 8192.0F;

			font_coords_buffer[k + 6] = q.s0 * 8192.0F;
			font_coords_buffer[k + 7] = q.t1 * 8192.0F;
			font_coords_buffer[k + 8] = q.s1 * 8192.0F;
			font_coords_buffer[k + 9] = q.t0 * 8192.0F;
			font_coords_buffer[k + 10] = q.s0 * 8192.0F;
			font_coords_buffer[k + 11] = q.t0 * 8192.0F;

			font_vertex_buffer[k + 0] = q.x0;
			font_vertex_buffer[k + 1] = -q.y1 + y;
			font_vertex_buffer[k + 2] = q.x1;
			font_vertex_buffer[k + 3] = -q.y1 + y;
			font_vertex_buffer[k + 4] = q.x1;
			font_vertex_buffer[k + 5] = -q.y0 + y;

			font_vertex_buffer[k + 6] = q.x0;
			font_vertex_buffer[k + 7] = -q.y1 + y;
			font_vertex_buffer[k + 8] = q.x1;
			font_vertex_buffer[k + 9] = -q.y0 + y;
			font_vertex_buffer[k + 10] = q.x0;
			font_vertex_buffer[k + 11] = -q.y0 + y;
			k += 12;
		}

		text++;
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef(1.0F / 8192.0F, 1.0F / 8192.0F, 1.0F);
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font->texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(2, GL_SHORT, 0, font_vertex_buffer);
	glTexCoordPointer(2, GL_SHORT, 0, font_coords_buffer);
	glDrawArrays(GL_TRIANGLES, 0, k / 2);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
}

void font_centered(float x, float y, float h, char* text) {
	font_render(x - font_length(h, text) / 2.0F, y, h, text);
}

