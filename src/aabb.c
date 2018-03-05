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

#include "common.h"

void aabb_render(AABB* a) {
	//glColor4f(1.0F,1.0F,1.0F,1.0F);
	glLineWidth(1.0F);
	glBegin(GL_LINES);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glEnd();
}

char aabb_intersection_ray(AABB* a, Ray* r) {
	double t1 = (a->min_x-r->origin.x)/r->direction.x;
    double t2 = (a->max_x-r->origin.x)/r->direction.x;

    double tmin = min(t1, t2);
    double tmax = max(t1, t2);

	t1 = (a->min_y - r->origin.y)/r->direction.y;
	t2 = (a->max_y - r->origin.y)/r->direction.y;
	tmin = max(tmin, min(min(t1, t2), tmax));
	tmax = min(tmax, max(max(t1, t2), tmin));

	t1 = (a->min_z - r->origin.z)/r->direction.z;
	t2 = (a->max_z - r->origin.z)/r->direction.z;
	tmin = max(tmin, min(min(t1, t2), tmax));
	tmax = min(tmax, max(max(t1, t2), tmin));

    return tmax > max(tmin, 0.0);
}

void aabb_set_center(AABB* a, float x, float y, float z) {
	float size_x = a->max_x-a->min_x;
	float size_y = a->max_y-a->min_y;
	float size_z = a->max_z-a->min_z;
	a->min_x = x-size_x/2;
	a->min_y = y-size_y/2;
	a->min_z = z-size_z/2;
	a->max_x = x+size_x/2;
	a->max_y = y+size_y/2;
	a->max_z = z+size_z/2;
}

void aabb_set_size(AABB* a, float x, float y, float z) {
	a->max_x = a->min_x+x;
	a->max_y = a->min_y+y;
	a->max_z = a->min_z+z;
}

boolean aabb_intersection(AABB* a, AABB* b) {
	return (a->min_x <= b->max_x && b->min_x <= a->max_x) && (a->min_y <= b->max_y && b->min_y <= a->max_y) && (a->min_z <= b->max_z && b->min_z <= a->max_z);
}

boolean aabb_intersection_terrain(AABB* a) {
	AABB terrain_cube;

	int min_x = min(max(floor(a->min_x)-1,0),map_size_x);
	int min_y = min(max(floor(a->min_y)-1,0),map_size_y);
	int min_z = min(max(floor(a->min_z)-1,0),map_size_z);

	int max_x = min(max(ceil(a->max_x)+1,0),map_size_x);
	int max_y = min(max(ceil(a->max_y)+1,0),map_size_y);
	int max_z = min(max(ceil(a->max_z)+1,0),map_size_z);

	for(int x=min_x;x<max_x;x++) {
		for(int z=min_z;z<max_z;z++) {
			for(int y=min_y;y<max_y;y++) {
				if(map_get(x,y,z)!=0xFFFFFFFF) {
					terrain_cube.min_x = x;
					terrain_cube.min_y = y;
					terrain_cube.min_z = z;
					terrain_cube.max_x = x+1;
					terrain_cube.max_y = y+1;
					terrain_cube.max_z = z+1;
					if(aabb_intersection(a,&terrain_cube)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}
