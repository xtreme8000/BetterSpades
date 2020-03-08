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

#ifndef FONT_H
#define FONT_H

#include "list.h"

#define FONT_FIXEDSYS 0
#define FONT_SMALLFNT 1

extern short* font_vertex_buffer;
extern short* font_coords_buffer;
extern int font_type;

extern struct list font_backed_fonts;

unsigned char font_init(void);
void font_reset(void);
float font_length(float h, char* text);
void font_render(float x, float y, float h, char* text);
void font_centered(float x, float y, float h, char* text);
void font_select(char type);

#endif
