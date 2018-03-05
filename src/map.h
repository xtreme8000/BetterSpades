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

extern unsigned int* map_colors;
extern int map_size_x;
extern int map_size_y;
extern int map_size_z;

extern float fog_color[4];
extern float map_sun[4];

extern unsigned char* map_minimap;

extern struct DamagedVoxel {
	int x,y,z;
	char damage;
	float timer;
} map_damaged_voxels[8];

struct Point {
	int x,y,z;
};

int map_damage(int x, int y, int z, int damage);
void map_damaged_voxels_render();
void map_update_physics(int x, int y, int z);
unsigned long long map_get(int x, int y, int z);
unsigned long long map_get_unblocked(int x, int y, int z);
void map_set(int x, int y, int z, unsigned long long color);
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned int* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned int* map);
void map_vxl_load(unsigned char* v, unsigned int* map);
void map_collapsing_render(float dt);
