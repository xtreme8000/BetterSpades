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

#include "tesselator.h"

void tesselator_create(struct tesselator* t, enum tesselator_type type) {
	t->quad_count = 0;
	t->quad_space = 128;
	t->vertices = NULL;
	t->colors = NULL;
	t->type = type;

	if(t->type == TESSELATE_QUADS) {
		t->vertices = malloc(t->quad_space * sizeof(int16_t) * 3 * 4);
		t->colors = malloc(t->quad_space * sizeof(uint32_t) * 4);
	}

	if(t->type == TESSELATE_TRIANGLES) {
		t->vertices = malloc(t->quad_space * sizeof(int16_t) * 3 * 6);
		t->colors = malloc(t->quad_space * sizeof(uint32_t) * 6);
	}
}

void tesselator_free(struct tesselator* t) {
	if(t->vertices)
		free(t->vertices);

	if(t->colors)
		free(t->colors);
}

void tesselator_glx(struct tesselator* t, struct glx_displaylist* x) {
	if(t->type == TESSELATE_QUADS) {
		glx_displaylist_update(x, t->quad_count * 4, GLX_DISPLAYLIST_NORMAL, t->colors, t->vertices, NULL);
	}

	if(t->type == TESSELATE_TRIANGLES) {
		glx_displaylist_update(x, t->quad_count * 6, GLX_DISPLAYLIST_NORMAL, t->colors, t->vertices, NULL);
	}
}

void tesselator_set_color(struct tesselator* t, uint32_t color) {
	t->color = color;
}

void tesselator_add(struct tesselator* t, int16_t* coords, uint32_t* colors) {
	if(t->quad_count >= t->quad_space) {
		t->quad_space *= 2;

		if(t->type == TESSELATE_QUADS) {
			t->vertices = realloc(t->vertices, t->quad_space * sizeof(int16_t) * 3 * 4);
			t->colors = realloc(t->colors, t->quad_space * sizeof(uint32_t) * 4);
		}

		if(t->type == TESSELATE_TRIANGLES) {
			t->vertices = realloc(t->vertices, t->quad_space * sizeof(int16_t) * 3 * 6);
			t->colors = realloc(t->colors, t->quad_space * sizeof(uint32_t) * 6);
		}
	}

	if(t->type == TESSELATE_QUADS) {
		memcpy(t->vertices + t->quad_count * 3 * 4, coords, sizeof(int16_t) * 3 * 4);
		memcpy(t->colors + t->quad_count * 4, colors, sizeof(uint32_t) * 4);
	}

	if(t->type == TESSELATE_TRIANGLES) {
		memcpy(t->vertices + t->quad_count * 3 * 6 + 3 * 0, coords, sizeof(int16_t) * 3 * 3);
		memcpy(t->vertices + t->quad_count * 3 * 6 + 3 * 3, coords + 3 * 0, sizeof(int16_t) * 3);
		memcpy(t->vertices + t->quad_count * 3 * 6 + 3 * 4, coords + 3 * 2, sizeof(int16_t) * 3);
		memcpy(t->vertices + t->quad_count * 3 * 6 + 3 * 5, coords + 3 * 3, sizeof(int16_t) * 3);

		memcpy(t->colors + t->quad_count * 6, colors, sizeof(uint32_t) * 3);
		t->colors[t->quad_count * 6 + 3] = colors[0];
		t->colors[t->quad_count * 6 + 4] = colors[2];
		t->colors[t->quad_count * 6 + 5] = colors[3];
	}

	t->quad_count++;
}

void tesselator_add_simple(struct tesselator* t, int16_t* coords) {
	tesselator_add(t, coords, (uint32_t[]) {t->color, t->color, t->color, t->color});
}
