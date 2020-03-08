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

#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <pthread.h>

#include "glx.h"
#include "tesselator.h"

#define CHUNK_SIZE 16
#define CHUNKS_PER_DIM 32

extern struct chunk {
	struct glx_displaylist display_list;
	int max_height;
	float last_update;
	int created;
} chunks[CHUNKS_PER_DIM * CHUNKS_PER_DIM];

extern int chunk_geometry_changed[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
extern int chunk_geometry_changed_lenght;

extern int chunk_lighting_changed[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
extern int chunk_lighting_changed_lenght;

extern int chunk_render_mode;

extern int chunk_enabled_cores;

#define CHUNK_WORKERS_MAX 16

#define CHUNK_WORKERSTATE_BUSY 0
#define CHUNK_WORKERSTATE_IDLE 1
#define CHUNK_WORKERSTATE_FINISHED 2

extern struct chunk_worker {
	int chunk_id;
	int chunk_x, chunk_y;
	pthread_mutex_t state_lock;
	pthread_cond_t can_work;
	int state;
	pthread_t thread;
	int max_height;
	struct tesselator tesselator;
	uint32_t minimap_data[CHUNK_SIZE * CHUNK_SIZE];
} chunk_workers[CHUNK_WORKERS_MAX];

extern pthread_rwlock_t* chunk_map_locks;

void chunk_init(void);

void chunk_block_update(int x, int y, int z);
void chunk_update_all(void);
void* chunk_generate(void* data);
void chunk_generate_greedy(int start_x, int start_z, struct tesselator* tess, int* max_height);
void chunk_generate_naive(int start_x, int start_z, struct tesselator* tess, int* max_height, int ao);
void chunk_render(int x, int y);
void chunk_rebuild_all(void);
void chunk_set_render_mode(int r);
void chunk_draw_visible(void);

void chunk_draw_shadow_volume(float* data, int max);
float chunk_face_light(float* data, int k, float lx, float ly, float lz);
int chunk_find_edge(float* data, int length, int exclude, float x1, float y1, float z1, float x2, float y2, float z2);

#endif
