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

// for future opengl es abstraction layer

#ifndef GLX_H
#define GLX_H

#include <stdint.h>
#include <stdbool.h>

extern int glx_version;
extern int glx_fog;

struct glx_displaylist {
	uint32_t legacy;
	uint32_t modern;
	size_t size;
	size_t buffer_size;
	bool has_normal;
	bool has_color;
};

enum {
	GLX_DISPLAYLIST_NORMAL,
	GLX_DISPLAYLIST_ENHANCED,
	GLX_DISPLAYLIST_POINTS,
};

void glx_init(void);

int glx_shader(const char* vertex, const char* fragment);

void glx_enable_sphericalfog(void);
void glx_disable_sphericalfog(void);

void glx_displaylist_create(struct glx_displaylist* x, bool has_color, bool has_normal);
void glx_displaylist_destroy(struct glx_displaylist* x);
void glx_displaylist_update(struct glx_displaylist* x, size_t size, int type, void* color, void* vertex, void* normal);
void glx_displaylist_draw(struct glx_displaylist* x, int type);

#endif
