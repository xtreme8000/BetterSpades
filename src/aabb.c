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

#include <float.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "map.h"
#include "aabb.h"
#include "tesselator.h"
#include "matrix.h"

void aabb_render(AABB* a) { }

// see: https://tavianator.com/2011/ray_box.html
bool aabb_intersection_ray(AABB* a, Ray* r, float* distance) {
	double inv_x = 1.0 / r->direction.x;
	double tx1 = (a->min_x - r->origin.x) * inv_x;
	double tx2 = (a->max_x - r->origin.x) * inv_x;

	double tmin = fmin(tx1, tx2);
	double tmax = fmax(tx1, tx2);

	double inv_y = 1.0 / r->direction.y;
	double ty1 = (a->min_y - r->origin.y) * inv_y;
	double ty2 = (a->max_y - r->origin.y) * inv_y;

	tmin = fmax(tmin, fmin(fmin(ty1, ty2), tmax));
	tmax = fmin(tmax, fmax(fmax(ty1, ty2), tmin));

	double inv_z = 1.0 / r->direction.z;
	double tz1 = (a->min_z - r->origin.z) * inv_z;
	double tz2 = (a->max_z - r->origin.z) * inv_z;

	tmin = fmax(tmin, fmin(fmin(tz1, tz2), tmax));
	tmax = fmin(tmax, fmax(fmax(tz1, tz2), tmin));

	if(distance)
		*distance = fmax(tmin, 0.0) * len3D(r->direction.x, r->direction.y, r->direction.z);

	return tmax > fmax(tmin, 0.0);
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

bool aabb_intersection(AABB* a, AABB* b) {
	return (a->min_x <= b->max_x && b->min_x <= a->max_x) && (a->min_y <= b->max_y && b->min_y <= a->max_y)
		&& (a->min_z <= b->max_z && b->min_z <= a->max_z);
}

bool aabb_intersection_terrain(AABB* a, int miny) {
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

					if(aabb_intersection(a, &terrain_cube))
						return true;
				}
			}
		}
	}

	return false;
}
