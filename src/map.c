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
#include <math.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "window.h"
#include "hashtable.h"
#include "sound.h"
#include "matrix.h"
#include "glx.h"
#include "chunk.h"
#include "common.h"
#include "map.h"
#include "camera.h"
#include "log.h"
#include "particle.h"
#include "minheap.h"

pthread_rwlock_t map_lock;

uint8_t* map_heights;
int map_size_x = 512;
int map_size_y = 64;
int map_size_z = 512;

struct libvxl_map map;

float fog_color[4] = {0.5F, 0.9098F, 1.0F, 1.0F};

float map_sun[4];

unsigned char* map_minimap;

struct DamagedVoxel map_damaged_voxels[8] = {0};

int map_object_visible(float x, float y, float z) {
	return !(x <= 0.0F && z <= 0.0F);
}

int map_damage(int x, int y, int z, int damage) {
	for(int k = 0; k < 8; k++) {
		if(window_time() - map_damaged_voxels[k].timer <= 10.0F && map_damaged_voxels[k].x == x
		   && map_damaged_voxels[k].y == y && map_damaged_voxels[k].z == z) {
			map_damaged_voxels[k].damage = min(damage + map_damaged_voxels[k].damage, 100);
			map_damaged_voxels[k].timer = window_time();
			return map_damaged_voxels[k].damage;
		}
	}
	int r = 0;
	for(int k = 0; k < 8; k++) {
		if(window_time() - map_damaged_voxels[k].timer > 10.0F) {
			r = k;
			break;
		}
	}
	map_damaged_voxels[r].x = x;
	map_damaged_voxels[r].y = y;
	map_damaged_voxels[r].z = z;
	map_damaged_voxels[r].damage = damage;
	map_damaged_voxels[r].timer = window_time();
	return damage;
}

int map_damage_get(int x, int y, int z) {
	for(int k = 0; k < 8; k++) {
		if(window_time() - map_damaged_voxels[k].timer <= 10.0F && map_damaged_voxels[k].x == x
		   && map_damaged_voxels[k].y == y && map_damaged_voxels[k].z == z) {
			return map_damaged_voxels[k].damage;
		}
	}
	return 0;
}

static void push_float3(float* buffer, int* index, float x, float y, float z) {
	buffer[(*index)++] = x;
	buffer[(*index)++] = y;
	buffer[(*index)++] = z;
}

static void push_char4(unsigned char* buffer, int* index, int r, int g, int b, int a) {
	buffer[(*index)++] = r;
	buffer[(*index)++] = g;
	buffer[(*index)++] = b;
	buffer[(*index)++] = a;
}

static void push_char46(unsigned char* buffer, int* index, int r, int g, int b, int a) {
	for(int k = 0; k < 6; k++) {
		push_char4(buffer, index, r, g, b, a);
	}
}

void map_damaged_voxels_render() {
	matrix_identity();
	matrix_upload();
	// glEnable(GL_POLYGON_OFFSET_FILL);
	// glPolygonOffset(0.0F,-100.0F);
	glDepthFunc(GL_EQUAL);
	glEnable(GL_BLEND);
	for(int k = 0; k < 8; k++) {
		if(map_isair(map_damaged_voxels[k].x, map_damaged_voxels[k].y, map_damaged_voxels[k].z)) {
			map_damaged_voxels[k].timer = 0;
		} else {
			if(window_time() - map_damaged_voxels[k].timer <= 10.0F) {
				float vertices[6 * 18];
				unsigned char colors[6 * 24];
				int color_index = 0, vertex_index = 0;

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);

				push_char46(colors, &color_index, 0, 0, 0, map_damaged_voxels[k].damage * 1.9125F);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y + 1,
							map_damaged_voxels[k].z + 1);
				push_float3(vertices, &vertex_index, map_damaged_voxels[k].x + 1, map_damaged_voxels[k].y,
							map_damaged_voxels[k].z + 1);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);
				glVertexPointer(3, GL_FLOAT, 0, vertices);
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, colors);
				glDrawArrays(GL_TRIANGLES, 0, vertex_index / 3);
				glDisableClientState(GL_COLOR_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
	}
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
	// glPolygonOffset(0.0F,0.0F);
	// glDisable(GL_POLYGON_OFFSET_FILL);
}

struct voxel {
	short x, y, z;
};

struct map_collapsing {
	HashTable voxels;
	struct Velocity v;
	struct Position p;
	struct Position p2;
	struct Orientation o;
	int voxel_count;
	int used, rotation, has_displaylist;
	struct glx_displaylist displaylist;
} map_collapsing_structures[32] = {0};

static int stack_contains(struct voxel* stack2, int len, int x, int y, int z) {
	for(int k = 0; k < len; k++)
		if(stack2[k].x == x && stack2[k].y == y && stack2[k].z == z)
			return 1;
	return 0;
}

static void map_update_physics_sub(int x, int y, int z) {
	if(y <= 1)
		return;

	struct minheap openlist;
	minheap_create(&openlist);
	HashTable closedlist;
	ht_setup(&closedlist, sizeof(uint32_t), sizeof(uint32_t), 256);
	closedlist.compare = int_cmp;
	closedlist.hash = int_hash;

	struct minheap_block start;
	start.pos = pos_key(x, y, z);
	minheap_put(&openlist, &start);

	while(!minheap_isempty(&openlist)) { // find all connected blocks
		struct minheap_block current = minheap_extract(&openlist);
		uint32_t color = map_get(pos_keyx(current.pos), pos_keyy(current.pos), pos_keyz(current.pos));
		ht_insert(&closedlist, &current.pos, &color);

		int x = pos_keyx(current.pos);
		int y = pos_keyy(current.pos);
		int z = pos_keyz(current.pos);

		int key;
		key = pos_key(x, y - 1, z);
		if(y >= 1 && !map_isair(x, y - 1, z) && !ht_contains(&closedlist, &key)) {
			if(y <= 2) { // reached layer at water level = finished!
				minheap_destroy(&openlist);
				ht_destroy(&closedlist);
				return;
			}
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});
		}

		key = pos_key(x, y + 1, z);
		if(y < map_size_y - 1 && !map_isair(x, y + 1, z) && !ht_contains(&closedlist, &key))
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});

		key = pos_key(x + 1, y, z);
		if(x < map_size_x - 1 && !map_isair(x + 1, y, z) && !ht_contains(&closedlist, &key))
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});

		key = pos_key(x - 1, y, z);
		if(y >= 1 && !map_isair(x - 1, y, z) && !ht_contains(&closedlist, &key))
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});

		key = pos_key(x, y, z + 1);
		if(z < map_size_z - 1 && !map_isair(x, y, z + 1) && !ht_contains(&closedlist, &key))
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});

		key = pos_key(x, y, z - 1);
		if(z >= 1 && !map_isair(x, y, z - 1) && !ht_contains(&closedlist, &key))
			minheap_put(&openlist, &(struct minheap_block) {.pos = key});
	}

	minheap_destroy(&openlist);

	float px = 0.0F, py = 0.0F, pz = 0.0F;
	for(int chain = 0; chain < closedlist.capacity; chain++) {
		HTNode* node = closedlist.nodes[chain];
		while(node) {
			uint32_t key = *(uint32_t*)(node->key);
			map_set(pos_keyx(key), pos_keyy(key), pos_keyz(key), 0xFFFFFFFF);
			px += pos_keyx(key);
			py += pos_keyy(key);
			pz += pos_keyz(key);

			node = node->next;
		}
	}
	px = (px / (float)closedlist.size) + 0.5F;
	py = (py / (float)closedlist.size) + 0.5F;
	pz = (pz / (float)closedlist.size) + 0.5F;

	sound_create(NULL, SOUND_WORLD, &sound_debris, px, py, pz);

	for(int k = 0; k < 32; k++) {
		if(!map_collapsing_structures[k].used) {
			map_collapsing_structures[k].used = 1;
			map_collapsing_structures[k].voxels = closedlist;
			map_collapsing_structures[k].v = (struct Velocity) {0, 0, 0};
			map_collapsing_structures[k].o = (struct Orientation) {0, 0, 0};
			map_collapsing_structures[k].p = (struct Position) {px, py, pz};
			map_collapsing_structures[k].p2 = (struct Position) {px, py, pz};
			map_collapsing_structures[k].rotation = rand() & 3;
			map_collapsing_structures[k].voxel_count = closedlist.size;
			map_collapsing_structures[k].has_displaylist = 0;
			return;
		}
	}

	// there was no free slot!
	ht_destroy(&closedlist);
}

int map_collapsing_cmp(const void* a, const void* b) {
	struct map_collapsing* A = (struct map_collapsing*)a;
	struct map_collapsing* B = (struct map_collapsing*)b;
	if(A->used && !B->used)
		return -1;
	if(!A->used && B->used)
		return 1;
	return distance3D(B->p.x, B->p.y, B->p.z, camera_x, camera_y, camera_z)
		- distance3D(A->p.x, A->p.y, A->p.z, camera_x, camera_y, camera_z);
}

void map_collapsing_render() {
	qsort(map_collapsing_structures, 32, sizeof(struct map_collapsing), map_collapsing_cmp);

	glEnable(GL_BLEND);
	for(int k = 0; k < 32; k++) {
		if(map_collapsing_structures[k].used) {
			matrix_push();
			matrix_identity();
			matrix_translate(map_collapsing_structures[k].p.x, map_collapsing_structures[k].p.y,
							 map_collapsing_structures[k].p.z);
			matrix_rotate(map_collapsing_structures[k].o.x, 1.0F, 0.0F, 0.0F);
			matrix_rotate(map_collapsing_structures[k].o.y, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			if(!map_collapsing_structures[k].has_displaylist) {
				map_collapsing_structures[k].has_displaylist = 1;
				glx_displaylist_create(&map_collapsing_structures[k].displaylist);

#ifdef OPENGL_ES
				float* vertices
					= malloc(map_collapsing_structures[k].voxel_count * 6 * 6 * 3 * sizeof(float)); // max space needed
				CHECK_ALLOCATION_ERROR(vertices)
				unsigned char* colors
					= malloc(map_collapsing_structures[k].voxel_count * 6 * 6 * 4 * sizeof(unsigned char));
				CHECK_ALLOCATION_ERROR(colors)
#else
				float* vertices = malloc(map_collapsing_structures[k].voxel_count * 6 * 4 * 3 * sizeof(float)); // same
				CHECK_ALLOCATION_ERROR(vertices)
				unsigned char* colors
					= malloc(map_collapsing_structures[k].voxel_count * 6 * 4 * 4 * sizeof(unsigned char));
				CHECK_ALLOCATION_ERROR(colors)
#endif

				int vertex_index = 0;
				int color_index = 0;

				for(int chain = 0; chain < map_collapsing_structures[k].voxels.capacity; chain++) {
					HTNode* node = map_collapsing_structures[k].voxels.nodes[chain];
					while(node) {
						uint32_t key = *(uint32_t*)(node->key);
						uint32_t color = *(uint32_t*)(node->value);

						int x2 = pos_keyx(key);
						int y2 = pos_keyy(key);
						int z2 = pos_keyz(key);

						float x = x2 - map_collapsing_structures[k].p2.x;
						float y = y2 - map_collapsing_structures[k].p2.y;
						float z = z2 - map_collapsing_structures[k].p2.z;

						key = pos_key(x2, y2 - 1, z2);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color) * 0.5F, green(color) * 0.5F,
										   blue(color) * 0.5F, 0xCC);
							}
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x + 1, y, z);
							push_float3(vertices, &vertex_index, x + 1, y, z + 1);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x + 1, y, z + 1);
#endif
							push_float3(vertices, &vertex_index, x, y, z + 1);
						}

						key = pos_key(x2, y2 + 1, z2);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color), green(color), blue(color), 0xCC);
							}
							push_float3(vertices, &vertex_index, x, y + 1, z);
							push_float3(vertices, &vertex_index, x, y + 1, z + 1);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x, y + 1, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#endif
							push_float3(vertices, &vertex_index, x + 1, y + 1, z);
						}

						key = pos_key(x2, y2, z2 - 1);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color) * 0.7F, green(color) * 0.7F,
										   blue(color) * 0.7F, 0xCC);
							}
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x, y + 1, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z);
#endif
							push_float3(vertices, &vertex_index, x + 1, y, z);
						}

						key = pos_key(x2, y2, z2 + 1);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color) * 0.6F, green(color) * 0.6F,
										   blue(color) * 0.6F, 0xCC);
							}
							push_float3(vertices, &vertex_index, x, y, z + 1);
							push_float3(vertices, &vertex_index, x + 1, y, z + 1);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x, y, z + 1);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#endif
							push_float3(vertices, &vertex_index, x, y + 1, z + 1);
						}

						key = pos_key(x2 - 1, y2, z2);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color) * 0.9F, green(color) * 0.9F,
										   blue(color) * 0.9F, 0xCC);
							}
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x, y, z + 1);
							push_float3(vertices, &vertex_index, x, y + 1, z + 1);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x, y, z);
							push_float3(vertices, &vertex_index, x, y + 1, z + 1);
#endif
							push_float3(vertices, &vertex_index, x, y + 1, z);
						}

						key = pos_key(x2 + 1, y2, z2);
						if(!ht_contains(&map_collapsing_structures[k].voxels, &key)) {
#ifdef OPENGL_ES
							for(int l = 0; l < 6; l++) {
#else
							for(int l = 0; l < 4; l++) {
#endif
								push_char4(colors, &color_index, red(color) * 0.8F, green(color) * 0.8F,
										   blue(color) * 0.8F, 0xCC);
							}
							push_float3(vertices, &vertex_index, x + 1, y, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#ifdef OPENGL_ES
							push_float3(vertices, &vertex_index, x + 1, y, z);
							push_float3(vertices, &vertex_index, x + 1, y + 1, z + 1);
#endif
							push_float3(vertices, &vertex_index, x + 1, y, z + 1);
						}

						node = node->next;
					}
				}
				glx_displaylist_update(&map_collapsing_structures[k].displaylist, vertex_index / 3,
									   GLX_DISPLAYLIST_ENHANCED, colors, vertices, NULL);
				free(colors);
				free(vertices);
			}
			glColorMask(0, 0, 0, 0);
			glx_displaylist_draw(&map_collapsing_structures[k].displaylist, GLX_DISPLAYLIST_ENHANCED);
			glColorMask(1, 1, 1, 1);
			glx_displaylist_draw(&map_collapsing_structures[k].displaylist, GLX_DISPLAYLIST_ENHANCED);

			matrix_pop();
		}
	}
	glDisable(GL_BLEND);
}

void map_collapsing_update(float dt) {
	for(int k = 0; k < 32; k++) {
		if(map_collapsing_structures[k].used) {
			map_collapsing_structures[k].v.y -= dt;

			matrix_push();
			matrix_identity();
			matrix_translate(map_collapsing_structures[k].p.x, map_collapsing_structures[k].p.y,
							 map_collapsing_structures[k].p.z);
			matrix_rotate(map_collapsing_structures[k].o.x, 1.0F, 0.0F, 0.0F);
			matrix_rotate(map_collapsing_structures[k].o.y, 0.0F, 1.0F, 0.0F);

			int collision = 0;
			for(int chain = 0; chain < map_collapsing_structures[k].voxels.capacity && !collision; chain++) {
				HTNode* node = map_collapsing_structures[k].voxels.nodes[chain];
				while(node) {
					uint32_t key = *(uint32_t*)(node->key);

					float v[4] = {pos_keyx(key) + map_collapsing_structures[k].v.x * dt * 32.0F
									  - map_collapsing_structures[k].p2.x + 0.5F,
								  pos_keyy(key) + map_collapsing_structures[k].v.y * dt * 32.0F
									  - map_collapsing_structures[k].p2.y + 0.5F,
								  pos_keyz(key) + map_collapsing_structures[k].v.z * dt * 32.0F
									  - map_collapsing_structures[k].p2.z + 0.5F,
								  1.0F};
					matrix_vector(v);
					if(!map_isair(v[0], v[1], v[2])) {
						collision = 1;
						break;
					}

					node = node->next;
				}
			}

			if(!collision) {
				map_collapsing_structures[k].p.x += map_collapsing_structures[k].v.x * dt * 32.0F;
				map_collapsing_structures[k].p.y += map_collapsing_structures[k].v.y * dt * 32.0F;
				map_collapsing_structures[k].p.z += map_collapsing_structures[k].v.z * dt * 32.0F;
			} else {
				map_collapsing_structures[k].v.x *= 0.85F;
				map_collapsing_structures[k].v.y *= -0.85F;
				map_collapsing_structures[k].v.z *= 0.85F;
				map_collapsing_structures[k].rotation++;
				map_collapsing_structures[k].rotation &= 3;
				sound_create(NULL, SOUND_WORLD, &sound_bounce, map_collapsing_structures[k].p.x,
							 map_collapsing_structures[k].p.y, map_collapsing_structures[k].p.z);

				if(absf(map_collapsing_structures[k].v.y) < 0.1F) {
					for(int chain = 0; chain < map_collapsing_structures[k].voxels.capacity; chain++) {
						HTNode* node = map_collapsing_structures[k].voxels.nodes[chain];
						while(node) {
							uint32_t key = *(uint32_t*)(node->key);
							uint32_t color = *(uint32_t*)(node->value);

							float v[4] = {pos_keyx(key) - map_collapsing_structures[k].p2.x + 0.5F,
										  pos_keyy(key) - map_collapsing_structures[k].p2.y + 0.5F,
										  pos_keyz(key) - map_collapsing_structures[k].p2.z + 0.5F, 1.0F};
							matrix_vector(v);
							particle_create(color, v[0], v[1], v[2], 2.5F, 1.0F, 2, 0.25F, 0.4F);

							node = node->next;
						}
					}

					matrix_pop();

					ht_destroy(&map_collapsing_structures[k].voxels);
					glx_displaylist_destroy(&map_collapsing_structures[k].displaylist);

					map_collapsing_structures[k].used = 0;
					continue;
				}
			}

			matrix_pop();

			map_collapsing_structures[k].o.x
				+= ((map_collapsing_structures[k].rotation & 1) ? 1.0F : -1.0F) * dt * 75.0F;
			map_collapsing_structures[k].o.y
				+= ((map_collapsing_structures[k].rotation & 2) ? 1.0F : -1.0F) * dt * 75.0F;
		}
	}
}

void map_update_physics(int x, int y, int z) {
	if(!map_isair(x + 1, y, z))
		map_update_physics_sub(x + 1, y, z);
	if(!map_isair(x - 1, y, z))
		map_update_physics_sub(x - 1, y, z);
	if(!map_isair(x, y, z + 1))
		map_update_physics_sub(x, y, z + 1);
	if(!map_isair(x, y, z - 1))
		map_update_physics_sub(x, y, z - 1);
	if(!map_isair(x, y - 1, z))
		map_update_physics_sub(x, y - 1, z);
	if(!map_isair(x, y + 1, z))
		map_update_physics_sub(x, y + 1, z);
}

// see this for details: https://github.com/infogulch/pyspades/blob/protocol075/pyspades/vxl_c.cpp#L380
float map_sunblock(int x, int y, int z) {
	int dec = 18;
	int i = 127;

	while(dec && y < 64) {
		if(!map_isair(x, ++y, --z))
			i -= dec;
		dec -= 2;
	}
	return (float)i / 127.0F;
}

void map_init() {
	libvxl_create(&map, 512, 512, 64, NULL, 0);
	pthread_rwlock_init(&map_lock, NULL);
}

int map_height_at(int x, int z) {
	int result[2];
	pthread_rwlock_rdlock(&map_lock);
	libvxl_map_gettop(&map, x, z, result);
	pthread_rwlock_unlock(&map_lock);
	return map_size_y - 1 - result[1];
}

int map_isair(int x, int y, int z) {
	pthread_rwlock_rdlock(&map_lock);
	int result = libvxl_map_issolid(&map, x, z, map_size_y - 1 - y);
	pthread_rwlock_unlock(&map_lock);
	return !result;
}

unsigned int map_get(int x, int y, int z) {
	pthread_rwlock_rdlock(&map_lock);
	unsigned int result = libvxl_map_get(&map, x, z, map_size_y - 1 - y);
	pthread_rwlock_unlock(&map_lock);
	return rgb2bgr(result);
}

void map_set(int x, int y, int z, unsigned int color) {
	if(x < 0 || y < 0 || z < 0 || x >= map_size_x || y >= map_size_y || z >= map_size_z)
		return;

	pthread_rwlock_wrlock(&map_lock);
	if(color == 0xFFFFFFFF) {
		libvxl_map_setair(&map, x, z, map_size_y - 1 - y);
	} else {
		libvxl_map_set(&map, x, z, map_size_y - 1 - y, rgb2bgr(color));
	}
	pthread_rwlock_unlock(&map_lock);

	chunk_block_update(x, y, z);

	int x_off = x % CHUNK_SIZE;
	int z_off = z % CHUNK_SIZE;

	if(x > 0 && x_off == 0)
		chunk_block_update(x - 1, y, z);
	if(z > 0 && z_off == 0)
		chunk_block_update(x, y, z - 1);
	if(x < map_size_x - 1 && x_off == CHUNK_SIZE - 1)
		chunk_block_update(x + 1, y, z);
	if(z < map_size_z - 1 && z_off == CHUNK_SIZE - 1)
		chunk_block_update(x, y, z + 1);
	if(x > 0 && z > 0 && x_off == 0 && z_off == 0)
		chunk_block_update(x - 1, y, z - 1);
	if(x < map_size_x - 1 && z < map_size_z - 1 && x_off == CHUNK_SIZE - 1 && z_off == CHUNK_SIZE - 1)
		chunk_block_update(x + 1, y, z + 1);
	if(x > 0 && z < map_size_z - 1 && x_off == 0 && z_off == CHUNK_SIZE - 1)
		chunk_block_update(x - 1, y, z + 1);
	if(x < map_size_x - 1 && z > 0 && x_off == CHUNK_SIZE - 1 && z_off == 0)
		chunk_block_update(x + 1, y, z - 1);

	if(x == 0)
		chunk_block_update(map_size_x - 1, y, z);
	if(x == map_size_x - 1)
		chunk_block_update(0, y, z);
	if(z == 0)
		chunk_block_update(x, y, map_size_z - 1);
	if(z == map_size_z - 1)
		chunk_block_update(x, y, 0);
}

// Copyright (c) Mathias Kaerlev 2011-2012 (but might be original code by Ben himself)
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array) {
	struct Point c, d;
	long ixi, iyi, izi, dx, dy, dz, dxi, dyi, dzi;
	int count = 0;

	// Note: positions MUST be rounded towards -inf
	c.x = x1;
	c.y = y1;
	c.z = z1;

	d.x = x2 - x1;
	d.y = y2 - y1;
	d.z = z2 - z1;

	if(d.x < 0)
		ixi = -1;
	else
		ixi = 1;
	if(d.y < 0)
		iyi = -1;
	else
		iyi = 1;
	if(d.z < 0)
		izi = -1;
	else
		izi = 1;

	if((abs(d.x) >= abs(d.y)) && (abs(d.x) >= abs(d.z))) {
		dxi = 1024;
		dx = 512;
		dyi = (long)(!d.y ? 0x3fffffff / 512 : abs(d.x * 1024 / d.y));
		dy = dyi / 2;
		dzi = (long)(!d.z ? 0x3fffffff / 512 : abs(d.x * 1024 / d.z));
		dz = dzi / 2;
	} else if(abs(d.y) >= abs(d.z)) {
		dyi = 1024;
		dy = 512;
		dxi = (long)(!d.x ? 0x3fffffff / 512 : abs(d.y * 1024 / d.x));
		dx = dxi / 2;
		dzi = (long)(!d.z ? 0x3fffffff / 512 : abs(d.y * 1024 / d.z));
		dz = dzi / 2;
	} else {
		dzi = 1024;
		dz = 512;
		dxi = (long)(!d.x ? 0x3fffffff / 512 : abs(d.z * 1024 / d.x));
		dx = dxi / 2;
		dyi = (long)(!d.y ? 0x3fffffff / 512 : abs(d.z * 1024 / d.y));
		dy = dyi / 2;
	}
	if(ixi >= 0)
		dx = dxi - dx;
	if(iyi >= 0)
		dy = dyi - dy;
	if(izi >= 0)
		dz = dzi - dz;

	while(1) {
		if(cube_array != NULL)
			cube_array[count] = c;

		if(count++ == 63)
			return count;

		if(c.x == x2 && c.y == y2 && c.z == z2)
			return count;

		if(dz <= dx && dz <= dy) {
			c.z += izi;
			if(c.z < 0 || c.z >= 64)
				return count;
			dz += dzi;
		} else {
			if(dx < dy) {
				c.x += ixi;
				if((unsigned long)c.x >= 512)
					return count;
				dx += dxi;
			} else {
				c.y += iyi;
				if((unsigned long)c.y >= 512)
					return count;
				dy += dyi;
			}
		}
	}
}

static int lerp(int a, int b, int amt) { // amt from 0 to 8
	return a + (b - a) * amt / 8;
}

static int dirt_color_table[]
	= {0x506050, 0x605848, 0x705040, 0x804838, 0x704030, 0x603828, 0x503020, 0x402818, 0x302010};

// thanks to BR_ for deobfuscating this method
int map_dirt_color(int x, int y, int z) {
	int vertical_slice = (map_size_y - 1 - y) / 8;
	int lerp_amt = (map_size_y - 1 - y) % 8;
	int dirt_base_color = dirt_color_table[vertical_slice];
	int dirt_next_color = dirt_color_table[vertical_slice + 1];

	int red = lerp(dirt_base_color & 0xFF0000, dirt_next_color & 0xFF0000, lerp_amt) >> 16;
	int green = lerp(dirt_base_color & 0x00FF00, dirt_next_color & 0x00FF00, lerp_amt) >> 8;
	int blue = lerp(dirt_base_color & 0x0000FF, dirt_next_color & 0x0000FF, lerp_amt);

	int rng = ms_rand() % 8;
	red += 4 * abs((x % 8) - 4) + rng;
	green += 4 * abs((z % 8) - 4) + rng;
	blue += 4 * abs(((map_size_y - 1 - y) % 8) - 4) + rng;

	return rgb(red, green, blue);
}

static int gkrand = 0;
int map_placedblock_color(int color) {
	color = color | 0x7F000000;
	gkrand = 0x1A4E86D * gkrand + 1;
	return color ^ (gkrand & 0x70707);
}

void map_vxl_load(void* v, size_t size) {
	pthread_rwlock_wrlock(&map_lock);
	libvxl_free(&map);
	libvxl_create(&map, 512, 512, 64, v, size);
	pthread_rwlock_unlock(&map_lock);
}

struct libvxl_block* map_copy_blocks(int chunk_x, int chunk_y, uint32_t* count) {
	struct libvxl_chunk* chunk
		= map.chunks + chunk_x + chunk_y * ((map_size_x + LIBVXL_CHUNK_SIZE - 1) / LIBVXL_CHUNK_SIZE);

	pthread_rwlock_rdlock(&map_lock);
	struct libvxl_block* blocks = malloc(chunk->index * sizeof(struct libvxl_block));
	CHECK_ALLOCATION_ERROR(blocks)
	memcpy(blocks, chunk->blocks, chunk->index * sizeof(struct libvxl_block));
	pthread_rwlock_unlock(&map_lock);

	if(count)
		*count = chunk->index;

	return blocks;
}

uint32_t* map_copy_solids() {
	pthread_rwlock_rdlock(&map_lock);
	size_t sg
		= (map.width * map.height * map.depth + (sizeof(uint32_t) * 8 - 1)) / (sizeof(uint32_t) * 8) * sizeof(uint32_t);
	uint32_t* blocks = malloc(sg);
	CHECK_ALLOCATION_ERROR(blocks)
	memcpy(blocks, map.geometry, sg);
	pthread_rwlock_unlock(&map_lock);

	return blocks;
}
