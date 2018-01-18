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

#define CHUNK_SIZE      16
#define CHUNKS_PER_DIM  32

extern struct chunk {
	int display_list;
	int max_height;
	float last_update;
	char created;
} chunks[CHUNKS_PER_DIM*CHUNKS_PER_DIM];

extern int chunk_geometry_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
extern int chunk_geometry_changed_lenght;

extern int chunk_lighting_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
extern int chunk_lighting_changed_lenght;

extern boolean chunk_render_mode;

#define CHUNK_WORKERS_MAX 4

#define CHUNK_WORKERSTATE_BUSY		0
#define CHUNK_WORKERSTATE_IDLE		1
#define CHUNK_WORKERSTATE_FINISHED	2

extern struct chunk_worker {
	int chunk_id;
	int chunk_x, chunk_y;
	int state;
	pthread_t thread;
	short* vertex_data;
	unsigned char* color_data;
	int data_size;
	int max_height;
} chunk_workers[CHUNK_WORKERS_MAX];
extern pthread_rwlock_t chunk_map_lock;

void chunk_init(void);

void chunk_block_update(int x, int y, int z);
void chunk_update_all(void);
void* chunk_generate(void* data);
void chunk_generate_greedy(struct chunk_worker* worker);
void chunk_generate_naive(struct chunk_worker* worker);
void chunk_render(int x, int y);
void chunk_rebuild_all(void);
void chunk_set_render_mode(boolean r);
void chunk_draw_visible(void);

void chunk_draw_shadow_volume(float* data, int max);
float chunk_face_light(float* data, int k, float lx, float ly, float lz);
int chunk_find_edge(float* data, int length, int exclude, float x1, float y1, float z1, float x2, float y2, float z2);
