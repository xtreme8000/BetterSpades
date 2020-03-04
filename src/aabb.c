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

#include "common.h"

void aabb_render(AABB* a) {}

float aabb_intersection_ray(AABB* a, Ray* r) {
	float total_min = FLT_MIN, total_max = FLT_MAX;

	for(int coord = 0; coord < 3; coord++) {
		float t_A = (a->min[coord] - r->origin.coords[coord]) / r->direction.coords[coord];
		float t_B = (a->max[coord] - r->origin.coords[coord]) / r->direction.coords[coord];

		float range_min = min(t_A, t_B);
		float range_max = max(t_A, t_B);

		// test if intervals don't intersect
		if(total_max <= range_min && total_min <= range_min)
			return -1;
		if(range_max <= total_max && range_max <= total_min)
			return -1;

		// now calculate overlap of intervals/ranges
		total_min = max(range_min, total_min);
		total_max = min(range_max, total_max);
	}

	if(total_min < 0)
		return -1;

	return absf(total_min) * len3D(r->direction.x, r->direction.y, r->direction.z);
}

void aabb_set_center(AABB* a, float x, float y, float z) {
	float size_x = a->max_x - a->min_x;
	float size_y = a->max_y - a->min_y;
	float size_z = a->max_z - a->min_z;
	a->min_x = x - size_x / 2;
	a->min_y = y - size_y / 2;
	a->min_z = z - size_z / 2;
	a->max_x = x + size_x / 2;
	a->max_y = y + size_y / 2;
	a->max_z = z + size_z / 2;
}

void aabb_set_size(AABB* a, float x, float y, float z) {
	a->max_x = a->min_x + x;
	a->max_y = a->min_y + y;
	a->max_z = a->min_z + z;
}

unsigned char aabb_intersection(AABB* a, AABB* b) {
	return (a->min_x <= b->max_x && b->min_x <= a->max_x) && (a->min_y <= b->max_y && b->min_y <= a->max_y)
		&& (a->min_z <= b->max_z && b->min_z <= a->max_z);
}

unsigned char aabb_intersection_terrain(AABB* a, int miny) {
	AABB terrain_cube;

	int min_x = min(max(floor(a->min_x) - 1, 0), map_size_x);
	int min_y = min(max(floor(a->min_y) - 1, miny), map_size_y);
	int min_z = min(max(floor(a->min_z) - 1, 0), map_size_z);

	int max_x = min(max(ceil(a->max_x) + 1, 0), map_size_x);
	int max_y = min(max(ceil(a->max_y) + 1, 0), map_size_y);
	int max_z = min(max(ceil(a->max_z) + 1, 0), map_size_z);

	for(int x = min_x; x < max_x; x++) {
		for(int z = min_z; z < max_z; z++) {
			for(int y = min_y; y < max_y; y++) {
				if(!map_isair(x, y, z)) {
					terrain_cube.min_x = x;
					terrain_cube.min_y = y;
					terrain_cube.min_z = z;
					terrain_cube.max_x = x + 1;
					terrain_cube.max_y = y + 1;
					terrain_cube.max_z = z + 1;
					if(aabb_intersection(a, &terrain_cube)) {
						return 1;
					}
				}
			}
		}
	}
	return 0;
}
