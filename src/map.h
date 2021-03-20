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

#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "libvxl.h"
#undef pos_key

extern int map_size_x;
extern int map_size_y;
extern int map_size_z;

extern float fog_color[4];

struct Point {
	int x, y, z;
};

void map_init();
int map_object_visible(float x, float y, float z);
int map_damage(int x, int y, int z, int damage);
int map_damage_get(int x, int y, int z);
void map_damaged_voxels_render();
void map_update_physics(int x, int y, int z);
float map_sunblock(int x, int y, int z);
int map_isair(int x, int y, int z);
unsigned int map_get(int x, int y, int z);
void map_set(int x, int y, int z, unsigned int color);
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned int* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned int* map);
int map_dirt_color(int x, int y, int z);
int map_placedblock_color(int color);
void map_vxl_load(void* v, size_t size);
void map_collapsing_render(void);
void map_collapsing_update(float dt);
int map_height_at(int x, int z);
struct libvxl_block* map_copy_blocks(int chunk_x, int chunk_y, uint32_t* count);
uint32_t* map_copy_solids(void);

#endif
