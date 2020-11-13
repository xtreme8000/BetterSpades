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
#include <string.h>
#include <stdint.h>
#include <float.h>

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
#include "tesselator.h"
#include "utils.h"
#include "config.h"
#include "channel.h"
#include "entitysystem.h"

int map_size_x = 512;
int map_size_y = 64;
int map_size_z = 512;

static struct libvxl_map map;
static pthread_rwlock_t map_lock;

float fog_color[4] = {0.5F, 0.9098F, 1.0F, 1.0F};

float map_sun[4];

struct damaged_voxel {
	int damage;
	float timer;
};

HashTable map_damaged_voxels;
struct tesselator map_damaged_tesselator;

int map_object_visible(float x, float y, float z) {
	return !(x <= 0.0F && z <= 0.0F);
}

int map_damage(int x, int y, int z, int damage) {
	uint32_t key = pos_key(x, y, z);
	struct damaged_voxel* voxel = ht_lookup(&map_damaged_voxels, &key);

	if(voxel) {
		voxel->damage = min(damage + voxel->damage, 100);
		voxel->timer = window_time();

		return voxel->damage;
	} else {
		ht_insert(&map_damaged_voxels, &key,
				  &(struct damaged_voxel) {
					  .damage = damage,
					  .timer = window_time(),
				  });

		return damage;
	}
}

int map_damage_get(int x, int y, int z) {
	uint32_t key = pos_key(x, y, z);
	struct damaged_voxel* voxel = ht_lookup(&map_damaged_voxels, &key);

	return voxel ? voxel->damage : 0;
}

static bool damaged_voxel_update(void* key, void* value, void* user) {
	uint32_t pos = *(uint32_t*)key;
	struct damaged_voxel* voxel = (struct damaged_voxel*)value;
	struct tesselator* tess = (struct tesselator*)user;
	int x = pos_keyx(pos);
	int y = pos_keyy(pos);
	int z = pos_keyz(pos);

	if(window_time() - voxel->timer > 10.0F || map_isair(x, y, z))
		return true;

	tesselator_set_color(tess, rgba(0, 0, 0, voxel->damage * 1.9125F));

	tesselator_addi_cube_face(tess, CUBE_FACE_Z_N, x, y, z);
	tesselator_addi_cube_face(tess, CUBE_FACE_Z_P, x, y, z);
	tesselator_addi_cube_face(tess, CUBE_FACE_X_N, x, y, z);
	tesselator_addi_cube_face(tess, CUBE_FACE_X_P, x, y, z);
	tesselator_addi_cube_face(tess, CUBE_FACE_Y_P, x, y, z);
	tesselator_addi_cube_face(tess, CUBE_FACE_Y_N, x, y, z);

	return false;
}

void map_damaged_voxels_render() {
	matrix_identity();
	matrix_upload();
	// glEnable(GL_POLYGON_OFFSET_FILL);
	// glPolygonOffset(0.0F,-100.0F);
	glDepthFunc(GL_EQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	tesselator_clear(&map_damaged_tesselator);

	ht_iterate_remove(&map_damaged_voxels, &map_damaged_tesselator, damaged_voxel_update);

	tesselator_draw(&map_damaged_tesselator, 1);

	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
	// glPolygonOffset(0.0F,0.0F);
	// glDisable(GL_POLYGON_OFFSET_FILL);
}

struct map_work_packet {
	int x, y, z;
};

struct channel map_work_queue;
struct channel map_result_queue;

struct map_collapsing {
	HashTable voxels;
	struct Velocity v;
	struct Position p;
	struct Position p2;
	struct Orientation o;
	int voxel_count;
	int rotation, has_displaylist;
	struct glx_displaylist displaylist;
	struct tesselator mesh_geometry;
};

struct entity_system map_collapsing_structures;

static bool falling_blocks_meshing(void* key, void* value, void* user) {
	uint32_t pos = *(uint32_t*)key;
	uint32_t color = *(uint32_t*)value;
	struct map_collapsing* collapsing = ((struct map_collapsing**)user)[0];
	struct tesselator* tess = ((struct tesselator**)user)[1];

	int x2 = pos_keyx(pos);
	int y2 = pos_keyy(pos);
	int z2 = pos_keyz(pos);

	float x = x2 - collapsing->p2.x;
	float y = y2 - collapsing->p2.y;
	float z = z2 - collapsing->p2.z;

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2, y2 - 1, z2)})) {
		tesselator_set_color(tess, rgba(red(color) * 0.5F, green(color) * 0.5F, blue(color) * 0.5F, 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_Y_N, x, y, z, 1.0F);
	}

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2, y2 + 1, z2)})) {
		tesselator_set_color(tess, rgba(red(color), green(color), blue(color), 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_Y_P, x, y, z, 1.0F);
	}

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2, y2, z2 - 1)})) {
		tesselator_set_color(tess, rgba(red(color) * 0.7F, green(color) * 0.7F, blue(color) * 0.7F, 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_Z_N, x, y, z, 1.0F);
	}

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2, y2, z2 + 1)})) {
		tesselator_set_color(tess, rgba(red(color) * 0.6F, green(color) * 0.6F, blue(color) * 0.6F, 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_Z_P, x, y, z, 1.0F);
	}

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2 - 1, y2, z2)})) {
		tesselator_set_color(tess, rgba(red(color) * 0.9F, green(color) * 0.9F, blue(color) * 0.9F, 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_X_N, x, y, z, 1.0F);
	}

	if(!ht_contains(&collapsing->voxels, (uint32_t[]) {pos_key(x2 + 1, y2, z2)})) {
		tesselator_set_color(tess, rgba(red(color) * 0.8F, green(color) * 0.8F, blue(color) * 0.8F, 0xCC));
		tesselator_addf_cube_face(tess, CUBE_FACE_X_P, x, y, z, 1.0F);
	}

	return true;
}

static bool falling_blocks_pivot(void* key, void* value, void* user) {
	float* pivot = (float*)user;
	uint32_t pos = *(uint32_t*)key;

	map_set(pos_keyx(pos), pos_keyy(pos), pos_keyz(pos), 0xFFFFFFFF);
	pivot[0] += pos_keyx(pos);
	pivot[1] += pos_keyy(pos);
	pivot[2] += pos_keyz(pos);

	return true;
}

static bool map_update_physics_sub(struct map_collapsing* collapsing, int x, int y, int z) {
	if(y <= 1)
		return false;

	if(map_isair(x, y, z))
		return false;

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

		if(y >= 1 && !map_isair(x, y - 1, z) && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x, y - 1, z)})) {
			if(y <= 2) { // reached layer at water level = finished!
				minheap_destroy(&openlist);
				ht_destroy(&closedlist);
				return false;
			}
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x, y - 1, z)});
		}

		if(y < map_size_y - 1 && !map_isair(x, y + 1, z)
		   && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x, y + 1, z)}))
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x, y + 1, z)});

		if(x < map_size_x - 1 && !map_isair(x + 1, y, z)
		   && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x + 1, y, z)}))
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x + 1, y, z)});

		if(y >= 1 && !map_isair(x - 1, y, z) && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x - 1, y, z)}))
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x - 1, y, z)});

		if(z < map_size_z - 1 && !map_isair(x, y, z + 1)
		   && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x, y, z + 1)}))
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x, y, z + 1)});

		if(z >= 1 && !map_isair(x, y, z - 1) && !ht_contains(&closedlist, (uint32_t[]) {pos_key(x, y, z - 1)}))
			minheap_put(&openlist, &(struct minheap_block) {.pos = pos_key(x, y, z - 1)});
	}

	minheap_destroy(&openlist);

	float pivot[3] = {0, 0, 0};
	ht_iterate(&closedlist, pivot, falling_blocks_pivot);

	for(size_t k = 0; k < 3; k++)
		pivot[k] = (pivot[k] / (float)closedlist.size) + 0.5F;

	collapsing->voxels = closedlist;
	collapsing->v = (struct Velocity) {0, 0, 0};
	collapsing->o = (struct Orientation) {0, 0, 0};
	collapsing->p = (struct Position) {pivot[0], pivot[1], pivot[2]};
	collapsing->p2 = collapsing->p;
	collapsing->rotation = rand() & 3;
	collapsing->voxel_count = closedlist.size;
	collapsing->has_displaylist = 0;

	tesselator_create(&collapsing->mesh_geometry, VERTEX_FLOAT, 0);
	ht_iterate(&collapsing->voxels, (void*[]) {collapsing, &collapsing->mesh_geometry}, falling_blocks_meshing);

	return true;
}

/*int map_collapsing_cmp(const void* a, const void* b) {
	struct map_collapsing* A = (struct map_collapsing*)a;
	struct map_collapsing* B = (struct map_collapsing*)b;
	if(A->used && !B->used)
		return -1;
	if(!A->used && B->used)
		return 1;
	return distance3D(B->p.x, B->p.y, B->p.z, camera_x, camera_y, camera_z)
		- distance3D(A->p.x, A->p.y, A->p.z, camera_x, camera_y, camera_z);
}*/

static bool falling_blocks_render(void* obj, void* user) {
	struct map_collapsing* collapsing = (struct map_collapsing*)obj;

	matrix_identity();
	matrix_translate(collapsing->p.x, collapsing->p.y, collapsing->p.z);
	matrix_rotate(collapsing->o.x, 1.0F, 0.0F, 0.0F);
	matrix_rotate(collapsing->o.y, 0.0F, 1.0F, 0.0F);
	matrix_upload();

	if(!collapsing->has_displaylist) {
		collapsing->has_displaylist = 1;
		glx_displaylist_create(&collapsing->displaylist, true, false);
		tesselator_glx(&collapsing->mesh_geometry, &collapsing->displaylist);
		tesselator_free(&collapsing->mesh_geometry);
	}

	glColorMask(0, 0, 0, 0);
	glx_displaylist_draw(&collapsing->displaylist, GLX_DISPLAYLIST_ENHANCED);
	glColorMask(1, 1, 1, 1);
	glx_displaylist_draw(&collapsing->displaylist, GLX_DISPLAYLIST_ENHANCED);

	return false;
}

void map_collapsing_render() {
	// qsort(map_collapsing_structures, 32, sizeof(struct map_collapsing), map_collapsing_cmp);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	matrix_push();
	entitysys_iterate(&map_collapsing_structures, NULL, falling_blocks_render);
	matrix_pop();

	glDisable(GL_BLEND);
}

static bool falling_blocks_collision(void* key, void* value, void* user) {
	uint32_t pos = *(uint32_t*)key;
	struct map_collapsing* collapsing = ((struct map_collapsing**)user)[0];
	float dt = *(((float**)user)[1]);

	float v[4] = {pos_keyx(pos) + collapsing->v.x * dt * 32.0F - collapsing->p2.x + 0.5F,
				  pos_keyy(pos) + collapsing->v.y * dt * 32.0F - collapsing->p2.y + 0.5F,
				  pos_keyz(pos) + collapsing->v.z * dt * 32.0F - collapsing->p2.z + 0.5F, 1.0F};

	matrix_vector(v);

	return map_isair(v[0], v[1], v[2]);
}

static bool falling_blocks_particles(void* key, void* value, void* user) {
	uint32_t pos = *(uint32_t*)key;
	uint32_t color = *(uint32_t*)value;
	struct map_collapsing* collapsing = (struct map_collapsing*)user;

	float v[4] = {pos_keyx(pos) - collapsing->p2.x + 0.5F, pos_keyy(pos) - collapsing->p2.y + 0.5F,
				  pos_keyz(pos) - collapsing->p2.z + 0.5F, 1.0F};
	matrix_vector(v);
	particle_create(color, v[0], v[1], v[2], 2.5F, 1.0F, 2, 0.25F, 0.4F);

	return true;
}

static bool falling_blocks_update(void* obj, void* user) {
	struct map_collapsing* collapsing = (struct map_collapsing*)obj;
	float dt = *(float*)user;

	collapsing->v.y -= dt;

	matrix_push();
	matrix_identity();
	matrix_translate(collapsing->p.x, collapsing->p.y, collapsing->p.z);
	matrix_rotate(collapsing->o.x, 1.0F, 0.0F, 0.0F);
	matrix_rotate(collapsing->o.y, 0.0F, 1.0F, 0.0F);

	bool collision = ht_iterate(&collapsing->voxels, (void*[]) {collapsing, &dt}, falling_blocks_collision);

	if(!collision) {
		collapsing->p.x += collapsing->v.x * dt * 32.0F;
		collapsing->p.y += collapsing->v.y * dt * 32.0F;
		collapsing->p.z += collapsing->v.z * dt * 32.0F;
	} else {
		collapsing->v.x *= 0.85F;
		collapsing->v.y *= -0.85F;
		collapsing->v.z *= 0.85F;
		collapsing->rotation++;
		collapsing->rotation &= 3;
		sound_create(SOUND_WORLD, &sound_bounce, collapsing->p.x, collapsing->p.y, collapsing->p.z);

		if(absf(collapsing->v.y) < 0.1F) {
			ht_iterate(&collapsing->voxels, collapsing, falling_blocks_particles);
			ht_destroy(&collapsing->voxels);

			if(collapsing->has_displaylist) {
				glx_displaylist_destroy(&collapsing->displaylist);
			} else {
				tesselator_free(&collapsing->mesh_geometry);
			}

			matrix_pop();
			return true;
		}
	}

	matrix_pop();

	collapsing->o.x += ((collapsing->rotation & 1) ? 1.0F : -1.0F) * dt * 75.0F;
	collapsing->o.y += ((collapsing->rotation & 2) ? 1.0F : -1.0F) * dt * 75.0F;

	return false;
}

void map_collapsing_update(float dt) {
	size_t drain = channel_size(&map_result_queue);

	for(size_t k = 0; k < drain; k++) {
		struct map_collapsing res;
		channel_await(&map_result_queue, &res);

		sound_create(SOUND_WORLD, &sound_debris, res.p.x, res.p.y, res.p.z);

		entitysys_add(&map_collapsing_structures, &res);
	}

	entitysys_iterate(&map_collapsing_structures, &dt, falling_blocks_update);
}

void map_update_physics(int x, int y, int z) {
	if(!map_isair(x + 1, y, z))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x + 1, .y = y, .z = z});
	if(!map_isair(x - 1, y, z))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x - 1, .y = y, .z = z});
	if(!map_isair(x, y, z + 1))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x, .y = y, .z = z + 1});
	if(!map_isair(x, y, z - 1))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x, .y = y, .z = z - 1});
	if(!map_isair(x, y - 1, z))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x, .y = y - 1, .z = z});
	if(!map_isair(x, y + 1, z))
		channel_put(&map_work_queue, &(struct map_work_packet) {.x = x, .y = y + 1, .z = z});
}

// see this for details: https://github.com/infogulch/pyspades/blob/protocol075/pyspades/vxl_c.cpp#L380
float map_sunblock(int x, int y, int z) {
	int dec = 18;
	int i = 127;

	while(dec && y < map_size_y) {
		if(!map_isair(x, ++y, --z))
			i -= dec;
		dec -= 2;
	}
	return (float)i / 127.0F;
}

void* falling_blocks_worker(void* user) {
	while(1) {
		struct map_work_packet work;
		channel_await(&map_work_queue, &work);

		struct map_collapsing collapsing;
		if(map_update_physics_sub(&collapsing, work.x, work.y, work.z))
			channel_put(&map_result_queue, &collapsing);
	}

	return NULL;
}

void map_init() {
	libvxl_create(&map, 512, 512, 64, NULL, 0);
	tesselator_create(&map_damaged_tesselator, VERTEX_INT, 0);
	pthread_rwlock_init(&map_lock, NULL);

	ht_setup(&map_damaged_voxels, sizeof(uint32_t), sizeof(struct damaged_voxel), 16);
	map_damaged_voxels.compare = int_cmp;
	map_damaged_voxels.hash = int_hash;

	entitysys_create(&map_collapsing_structures, sizeof(struct map_collapsing), 32);

	channel_create(&map_work_queue, sizeof(struct map_work_packet), 16);
	channel_create(&map_result_queue, sizeof(struct map_collapsing), 16);

	pthread_t worker;
	pthread_create(&worker, NULL, falling_blocks_worker, NULL);
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
	int result = !libvxl_map_issolid(&map, x, z, map_size_y - 1 - y);
	pthread_rwlock_unlock(&map_lock);

	return result;
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

	if(settings.ambient_occlusion) {
		if(x > 0 && z > 0 && x_off == 0 && z_off == 0)
			chunk_block_update(x - 1, y, z - 1);
		if(x < map_size_x - 1 && z < map_size_z - 1 && x_off == CHUNK_SIZE - 1 && z_off == CHUNK_SIZE - 1)
			chunk_block_update(x + 1, y, z + 1);
		if(x > 0 && z < map_size_z - 1 && x_off == 0 && z_off == CHUNK_SIZE - 1)
			chunk_block_update(x - 1, y, z + 1);
		if(x < map_size_x - 1 && z > 0 && x_off == CHUNK_SIZE - 1 && z_off == 0)
			chunk_block_update(x + 1, y, z - 1);
	}

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
	pthread_rwlock_rdlock(&map_lock);

	struct libvxl_chunk* chunk
		= map.chunks + chunk_x + chunk_y * ((map_size_x + LIBVXL_CHUNK_SIZE - 1) / LIBVXL_CHUNK_SIZE);

	struct libvxl_block* blocks = malloc(chunk->index * sizeof(struct libvxl_block));
	CHECK_ALLOCATION_ERROR(blocks)
	memcpy(blocks, chunk->blocks, chunk->index * sizeof(struct libvxl_block));

	if(count)
		*count = chunk->index;

	pthread_rwlock_unlock(&map_lock);

	return blocks;
}

uint32_t* map_copy_solids() {
	size_t sg
		= (map.width * map.height * map.depth + (sizeof(uint32_t) * 8 - 1)) / (sizeof(uint32_t) * 8) * sizeof(uint32_t);
	uint32_t* blocks = malloc(sg);
	CHECK_ALLOCATION_ERROR(blocks)
	pthread_rwlock_rdlock(&map_lock);
	memcpy(blocks, map.geometry, sg);
	pthread_rwlock_unlock(&map_lock);

	return blocks;
}
