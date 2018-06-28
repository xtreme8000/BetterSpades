/*
	Copyright (c) 2017-2018 ByteBit

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

typedef struct {
	float min_x, min_y, min_z;
	float max_x, max_y, max_z;
} AABB;

typedef struct {
	struct {
		float x,y,z;
	} origin;
	struct {
		float x,y,z;
	} direction;
} Ray;

unsigned char aabb_intersection(AABB* a, AABB* b);
char aabb_intersection_ray(AABB* a, Ray* r);
unsigned char aabb_intersection_terrain(AABB* a, int miny);
void aabb_set_size(AABB* a, float x, float y, float z);
void aabb_set_center(AABB* a, float x, float y, float z);
void aabb_render(AABB* a);
