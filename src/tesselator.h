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

enum tesselator_type {
	TESSELATE_QUADS,
	TESSELATE_TRIANGLES,
};

struct tesselator {
	int16_t* vertices;
	uint32_t* colors;
	uint32_t quad_count;
	uint32_t quad_space;
	uint32_t color;
	enum tesselator_type type;
};

void tesselator_create(struct tesselator* t, enum tesselator_type type);
void tesselator_free(struct tesselator* t);
void tesselator_glx(struct tesselator* t, struct glx_displaylist* x);
void tesselator_set_color(struct tesselator* t, uint32_t color);
void tesselator_add(struct tesselator* t, int16_t* coords, uint32_t* colors);
void tesselator_add_simple(struct tesselator* t, int16_t* coords);

#endif
