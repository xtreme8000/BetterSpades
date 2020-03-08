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

#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>

#include "common.h"
#include "window.h"
#include "config.h"
#include "texture.h"
#include "log.h"
#include "matrix.h"
#include "map.h"
#include "camera.h"
#include "tesselator.h"
#include "chunk.h"

struct chunk chunks[CHUNKS_PER_DIM * CHUNKS_PER_DIM] = {0};

int chunk_geometry_changed[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
int chunk_geometry_changed_lenght = 0;

int chunk_lighting_changed[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
int chunk_lighting_changed_lenght = 0;

int chunk_render_mode = 0;

struct chunk_d {
	int x, y;
};

struct chunk_worker chunk_workers[CHUNK_WORKERS_MAX];

int chunk_enabled_cores;

pthread_rwlock_t* chunk_map_locks;

static int chunk_sort(const void* a, const void* b) {
	struct chunk_d* aa = (struct chunk_d*)a;
	struct chunk_d* bb = (struct chunk_d*)b;
	return distance2D(aa->x + CHUNK_SIZE / 2, aa->y + CHUNK_SIZE / 2, camera_x, camera_z)
		- distance2D(bb->x + CHUNK_SIZE / 2, bb->y + CHUNK_SIZE / 2, camera_x, camera_z);
}

void chunk_init() {
	chunk_enabled_cores = min(max(window_cpucores() / 2, 1), CHUNK_WORKERS_MAX);
	log_info("%i cores enabled for chunk generation", chunk_enabled_cores);
	chunk_map_locks = malloc(map_size_x * map_size_z * sizeof(pthread_rwlock_t));
	CHECK_ALLOCATION_ERROR(chunk_map_locks);
	for(int k = 0; k < map_size_x * map_size_z; k++)
		pthread_rwlock_init(&chunk_map_locks[k], NULL);
	for(int k = 0; k < chunk_enabled_cores; k++) {
		chunk_workers[k].state = CHUNK_WORKERSTATE_IDLE;
		pthread_mutex_init(&chunk_workers[k].state_lock, NULL);
		pthread_cond_init(&chunk_workers[k].can_work, NULL);
		pthread_create(&chunk_workers[k].thread, NULL, chunk_generate, &chunk_workers[k]);
	}
}

void chunk_draw_visible() {
	struct chunk_d chunks_draw[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
	int index = 0;

	// go through all possible chunks and store all in range and view
	for(int y = -9; y < CHUNKS_PER_DIM + 9; y++) {
		for(int x = -9; x < CHUNKS_PER_DIM + 9; x++) {
			if(distance2D((x + 0.5F) * CHUNK_SIZE, (y + 0.5F) * CHUNK_SIZE, camera_x, camera_z)
			   <= pow(settings.render_distance + 1.414F * CHUNK_SIZE, 2)) {
				int tmp_x = x, tmp_y = y;
				if(tmp_x < 0)
					tmp_x += CHUNKS_PER_DIM;
				if(tmp_y < 0)
					tmp_y += CHUNKS_PER_DIM;
				if(tmp_x >= CHUNKS_PER_DIM)
					tmp_x -= CHUNKS_PER_DIM;
				if(tmp_y >= CHUNKS_PER_DIM)
					tmp_y -= CHUNKS_PER_DIM;

				if(camera_CubeInFrustum((x + 0.5F) * CHUNK_SIZE, 0.0F, (y + 0.5F) * CHUNK_SIZE, CHUNK_SIZE / 2,
										chunks[tmp_y * CHUNKS_PER_DIM + tmp_x].max_height))
					chunks_draw[index++] = (struct chunk_d) {x * CHUNK_SIZE, y * CHUNK_SIZE};
			}
		}
	}

	// sort all chunks to draw those in front first
	qsort(chunks_draw, index, sizeof(struct chunk_d), chunk_sort);

	for(int k = 0; k < index; k++)
		chunk_render(chunks_draw[k].x / CHUNK_SIZE, chunks_draw[k].y / CHUNK_SIZE);
}

void chunk_set_render_mode(int r) {
	chunk_render_mode = r;
}

void chunk_rebuild_all() {
	for(int d = 0; d < CHUNKS_PER_DIM * CHUNKS_PER_DIM; d++) {
		chunk_geometry_changed[d] = d;
	}
	chunk_geometry_changed_lenght = CHUNKS_PER_DIM * CHUNKS_PER_DIM;
}

void chunk_render(int x, int y) {
	matrix_push();
	matrix_translate((x < 0) * -map_size_x + (x >= CHUNKS_PER_DIM) * map_size_x, 0.0F,
					 (y < 0) * -map_size_z + (y >= CHUNKS_PER_DIM) * map_size_z);
	matrix_upload();

	if(x < 0)
		x += CHUNKS_PER_DIM;
	if(y < 0)
		y += CHUNKS_PER_DIM;
	if(x >= CHUNKS_PER_DIM)
		x -= CHUNKS_PER_DIM;
	if(y >= CHUNKS_PER_DIM)
		y -= CHUNKS_PER_DIM;

	// glPolygonMode(GL_FRONT, GL_LINE);

	if(chunks[((y * CHUNKS_PER_DIM) | x)].created)
		glx_displaylist_draw(&chunks[((y * CHUNKS_PER_DIM) | x)].display_list, GLX_DISPLAYLIST_NORMAL);

	// glPolygonMode(GL_FRONT, GL_FILL);

	matrix_pop();
}

void* chunk_generate(void* data) {
	pthread_detach(pthread_self());

	struct chunk_worker* worker = (struct chunk_worker*)data;

	while(1) {
		pthread_mutex_lock(&worker->state_lock);
		worker->state = CHUNK_WORKERSTATE_FINISHED;

		while(worker->state != CHUNK_WORKERSTATE_BUSY)
			pthread_cond_wait(&worker->can_work, &worker->state_lock);
		pthread_mutex_unlock(&worker->state_lock);

		if(settings.greedy_meshing)
			chunk_generate_greedy(worker->chunk_x, worker->chunk_y, &worker->tesselator, &worker->max_height);
		else
			chunk_generate_naive(worker->chunk_x, worker->chunk_y, &worker->tesselator, &worker->max_height,
								 settings.ambient_occlusion);

		for(int x = worker->chunk_x; x < worker->chunk_x + CHUNK_SIZE; x++) {
			for(int z = worker->chunk_y; z < worker->chunk_y + CHUNK_SIZE; z++) {
				uint32_t* out = worker->minimap_data + (x - worker->chunk_x + (z - worker->chunk_y) * CHUNK_SIZE);
				if((x % 64) > 0 && (z % 64) > 0) {
					pthread_rwlock_rdlock(&chunk_map_locks[x + z * map_size_x]);
					uint32_t color = map_colors[x + (map_heights[x + z * map_size_x] * map_size_z + z) * map_size_x];
					pthread_rwlock_unlock(&chunk_map_locks[x + z * map_size_x]);

					*out = rgba(red(color), green(color), blue(color), 255);
				} else {
					*out = rgba(255, 255, 255, 255);
				}
			}
		}
	}

	return NULL;
}

void chunk_generate_greedy(int start_x, int start_z, struct tesselator* tess, int* max_height) {
	*max_height = 0;

	int checked_voxels[2][CHUNK_SIZE * CHUNK_SIZE];
	int checked_voxels2[2][CHUNK_SIZE * map_size_y];

	for(int z = start_z; z < start_z + CHUNK_SIZE; z++) {
		memset(checked_voxels2[0], 0, sizeof(int) * CHUNK_SIZE * map_size_y);
		memset(checked_voxels2[1], 0, sizeof(int) * CHUNK_SIZE * map_size_y);

		for(int x = start_x; x < start_x + CHUNK_SIZE; x++) {
			for(int y = 0; y < map_size_y; y++) {
				if(!map_isair(x, y, z)) {
					if(*max_height < y) {
						*max_height = y;
					}

					unsigned int col = map_get(x, y, z);
					int r = red(col);
					int g = green(col);
					int b = blue(col);

					if((z == 0 && map_isair(x, y, map_size_z - 1)) || (z > 0 && map_isair(x, y, z - 1))) {
						if(checked_voxels2[0][y + (x - start_x) * map_size_y] == 0) {
							int len_y = 0;
							int len_x = 1;

							for(int a = 0; a < map_size_y - y; a++) {
								if(map_get(x, y + a, z) == col
								   && checked_voxels2[0][y + a + (x - start_x) * map_size_y] == 0
								   && ((z == 0 && map_isair(x, y + a, map_size_z - 1))
									   || (z > 0 && map_isair(x, y + a, z - 1))))
									len_y++;
								else
									break;
							}

							for(int b = 1; b < (start_x + CHUNK_SIZE - x); b++) {
								int a;
								for(a = 0; a < len_y; a++) {
									if(map_get(x + b, y + a, z) != col
									   || checked_voxels2[0][y + a + (x + b - start_x) * map_size_y] != 0
									   || !((z == 0 && map_isair(x + b, y + a, map_size_z - 1))
											|| (z > 0 && map_isair(x + b, y + a, z - 1))))
										break;
								}
								if(a == len_y)
									len_x++;
								else
									break;
							}

							for(int b = 0; b < len_x; b++)
								for(int a = 0; a < len_y; a++)
									checked_voxels2[0][y + a + (x + b - start_x) * map_size_y] = 1;

							tesselator_set_color(tess, rgba(r * 0.875F, g * 0.875F, b * 0.875F, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x, y, z, x, y + len_y, z, x + len_x, y + len_y, z, x + len_x, y, z});
						}
					}

					if((z == map_size_z - 1 && map_isair(x, y, 0)) || (z < map_size_z - 1 && map_isair(x, y, z + 1))) {
						if(checked_voxels2[1][y + (x - start_x) * map_size_y] == 0) {
							int len_y = 0;
							int len_x = 1;

							for(int a = 0; a < map_size_y - y; a++) {
								if(map_get(x, y + a, z) == col
								   && checked_voxels2[1][y + a + (x - start_x) * map_size_y] == 0
								   && ((z == map_size_z - 1 && map_isair(x, y + a, 0))
									   || (z < map_size_z - 1 && map_isair(x, y + a, z + 1))))
									len_y++;
								else
									break;
							}

							for(int b = 1; b < (start_x + CHUNK_SIZE - x); b++) {
								int a;
								for(a = 0; a < len_y; a++) {
									if(map_get(x + b, a + a, z) != col
									   || checked_voxels2[1][y + a + (x + b - start_x) * map_size_y] != 0
									   || !((z == map_size_z - 1 && map_isair(x + b, y + a, 0))
											|| (z < map_size_z - 1 && map_isair(x + b, y + a, z + 1))))
										break;
								}
								if(a == len_y)
									len_x++;
								else
									break;
							}

							for(int b = 0; b < len_x; b++)
								for(int a = 0; a < len_y; a++)
									checked_voxels2[1][y + a + (x + b - start_x) * map_size_y] = 1;

							tesselator_set_color(tess, rgba(r * 0.625F, g * 0.625F, b * 0.625F, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x, y, z + 1, x + len_x, y, z + 1, x + len_x, y + len_y,
															   z + 1, x, y + len_y, z + 1});
						}
					}
				}
			}
		}
	}

	for(int x = start_x; x < start_x + CHUNK_SIZE; x++) {
		memset(checked_voxels2[0], 0, sizeof(int) * CHUNK_SIZE * map_size_y);
		memset(checked_voxels2[1], 0, sizeof(int) * CHUNK_SIZE * map_size_y);

		for(int z = start_z; z < start_z + CHUNK_SIZE; z++) {
			for(int y = 0; y < map_size_y; y++) {
				if(!map_isair(x, y, z)) {
					if(*max_height < y) {
						*max_height = y;
					}

					unsigned int col = map_get(x, y, z);
					int r = red(col);
					int g = green(col);
					int b = blue(col);

					if((x == 0 && map_isair(map_size_x - 1, y, z)) || (x > 0 && map_isair(x - 1, y, z))) {
						if(checked_voxels2[0][y + (z - start_z) * map_size_y] == 0) {
							int len_y = 0;
							int len_z = 1;

							for(int a = 0; a < map_size_y - y; a++) {
								if(map_get(x, y + a, z) == col
								   && checked_voxels2[0][y + a + (z - start_z) * map_size_y] == 0
								   && ((x == 0 && map_isair(map_size_x - 1, y + a, z))
									   || (x > 0 && map_isair(x - 1, y + a, z))))
									len_y++;
								else
									break;
							}

							for(int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
								int a;
								for(a = 0; a < len_y; a++) {
									if(map_get(x, y + a, z + b) != col
									   || checked_voxels2[0][y + a + (z + b - start_z) * map_size_y] != 0
									   || !((x == 0 && map_isair(map_size_x - 1, y + a, z + b))
											|| (x > 0 && map_isair(x - 1, y + a, z + b))))
										break;
								}
								if(a == len_y)
									len_z++;
								else
									break;
							}

							for(int b = 0; b < len_z; b++)
								for(int a = 0; a < len_y; a++)
									checked_voxels2[0][y + a + (z + b - start_z) * map_size_y] = 1;

							tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x, y, z, x, y, z + len_z, x, y + len_y, z + len_z, x, y + len_y, z});
						}
					}

					if((x == map_size_x - 1 && map_isair(0, y, z)) || (x < map_size_x - 1 && map_isair(x + 1, y, z))) {
						if(checked_voxels2[1][y + (z - start_z) * map_size_y] == 0) {
							int len_y = 0;
							int len_z = 1;

							for(int a = 0; a < map_size_y - y; a++) {
								if(map_get(x, y + a, z) == col
								   && checked_voxels2[1][y + a + (z - start_z) * map_size_y] == 0
								   && ((x == map_size_x - 1 && map_isair(0, y + a, z))
									   || (x < map_size_x - 1 && map_isair(x + 1, y + a, z))))
									len_y++;
								else
									break;
							}

							for(int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
								int a;
								for(a = 0; a < len_y; a++) {
									if(map_get(x, y + a, z + b) != col
									   || checked_voxels2[1][y + a + (z + b - start_z) * map_size_y] != 0
									   || !((x == map_size_x - 1 && map_isair(0, y + a, z + b))
											|| (x < map_size_x - 1 && map_isair(x + 1, y + a, z + b))))
										break;
								}
								if(a == len_y)
									len_z++;
								else
									break;
							}

							for(unsigned char b = 0; b < len_z; b++)
								for(unsigned char a = 0; a < len_y; a++)
									checked_voxels2[1][y + a + (z + b - start_z) * map_size_y] = 1;

							tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x + 1, y, z, x + 1, y + len_y, z, x + 1, y + len_y,
															   z + len_z, x + 1, y, z + len_z});
						}
					}
				}
			}
		}
	}

	for(int y = 0; y < map_size_y; y++) {
		memset(checked_voxels[0], 0, sizeof(int) * CHUNK_SIZE * CHUNK_SIZE);
		memset(checked_voxels[1], 0, sizeof(int) * CHUNK_SIZE * CHUNK_SIZE);

		for(int x = start_x; x < start_x + CHUNK_SIZE; x++) {
			for(int z = start_z; z < start_z + CHUNK_SIZE; z++) {
				if(!map_isair(x, y, z)) {
					if(*max_height < y) {
						*max_height = y;
					}

					unsigned int col = map_get(x, y, z);
					int r = red(col);
					int g = green(col);
					int b = blue(col);

					if(y == map_size_y - 1 || map_isair(x, y + 1, z)) {
						if(checked_voxels[0][(x - start_x) + (z - start_z) * CHUNK_SIZE] == 0) {
							int len_x = 0;
							int len_z = 1;

							for(int a = 0; a < (start_x + CHUNK_SIZE - x); a++) {
								if(map_get(x + a, y, z) == col
								   && checked_voxels[0][(x + a - start_x) + (z - start_z) * CHUNK_SIZE] == 0
								   && (y == map_size_y - 1 || map_isair(x + a, y + 1, z)))
									len_x++;
								else
									break;
							}

							for(int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
								int a;
								for(a = 0; a < len_x; a++) {
									if(map_get(x + a, y, z + b) != col
									   || checked_voxels[0][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] != 0
									   || !(y == map_size_y - 1 || map_isair(x + a, y + 1, z + b)))
										break;
								}
								if(a == len_x)
									len_z++;
								else
									break;
							}

							for(int b = 0; b < len_z; b++)
								for(int a = 0; a < len_x; a++)
									checked_voxels[0][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] = 1;

							tesselator_set_color(tess, rgba(r, g, b, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x, y + 1, z, x, y + 1, z + len_z, x + len_x, y + 1,
															   z + len_z, x + len_x, y + 1, z});
						}
					}

					if(y > 0 && map_isair(x, y - 1, z)) {
						if(checked_voxels[1][(x - start_x) + (z - start_z) * CHUNK_SIZE] == 0) {
							int len_x = 0;
							int len_z = 1;

							for(int a = 0; a < (start_x + CHUNK_SIZE - x); a++) {
								if(map_get(x + a, y, z) == col
								   && checked_voxels[1][(x + a - start_x) + (z - start_z) * CHUNK_SIZE] == 0
								   && (y > 0 && map_isair(x + a, y - 1, z)))
									len_x++;
								else
									break;
							}

							for(int b = 1; b < (start_z + CHUNK_SIZE - z); b++) {
								int a;
								for(a = 0; a < len_x; a++) {
									if(map_get(x + a, y, z + b) != col
									   || checked_voxels[1][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] != 0
									   || !(y > 0 && map_isair(x + a, y - 1, z + b)))
										break;
								}
								if(a == len_x)
									len_z++;
								else
									break;
							}

							for(int b = 0; b < len_z; b++)
								for(int a = 0; a < len_x; a++)
									checked_voxels[1][(x + a - start_x) + (z + b - start_z) * CHUNK_SIZE] = 1;

							tesselator_set_color(tess, rgba(r * 0.5F, g * 0.5F, b * 0.5F, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x, y, z, x + len_x, y, z, x + len_x, y, z + len_z, x, y, z + len_z});
						}
					}
				}
			}
		}
	}

	(*max_height)++;
}

//+X = 0.75
//-X = 0.75
//+Y = 1.0
//-Y = 0.5
//+Z = 0.625
//-Z = 0.875

// credit: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
static float vertexAO(int side1, int side2, int corner) {
	if(!side1 && !side2) {
		return 0.25F;
	}

	return 0.75F - (!side1 + !side2 + !corner) * 0.25F + 0.25F;
}

void chunk_generate_naive(int start_x, int start_z, struct tesselator* tess, int* max_height, int ao) {
	*max_height = 0;

	for(int z = start_z; z < start_z + CHUNK_SIZE; z++) {
		for(int x = start_x; x < start_x + CHUNK_SIZE; x++) {
			for(int y = 0; y < map_size_y; y++) {
				if(!map_isair(x, y, z)) {
					if(*max_height < y) {
						*max_height = y;
					}

					uint32_t col = map_get(x, y, z);
					int r = red(col);
					int g = green(col);
					int b = blue(col);

					float shade = map_sunblock(x, y, z);
					r *= shade;
					g *= shade;
					b *= shade;

					if((z == 0 && map_isair(x, y, map_size_z - 1)) || map_isair(x, y, z - 1)) {
						if(ao) {
							float A = vertexAO(map_isair(x - 1, y, z - 1), map_isair(x, y - 1, z - 1),
											   map_isair(x - 1, y - 1, z - 1));
							float B = vertexAO(map_isair(x - 1, y, z - 1), map_isair(x, y + 1, z - 1),
											   map_isair(x - 1, y + 1, z - 1));
							float C = vertexAO(map_isair(x + 1, y, z - 1), map_isair(x, y + 1, z - 1),
											   map_isair(x + 1, y + 1, z - 1));
							float D = vertexAO(map_isair(x + 1, y, z - 1), map_isair(x, y - 1, z - 1),
											   map_isair(x + 1, y - 1, z - 1));

							tesselator_add(tess, (int16_t[]) {x, y, z, x, y + 1, z, x + 1, y + 1, z, x + 1, y, z},
										   (uint32_t[]) {
											   rgba(r * 0.875F * A, g * 0.875F * A, b * 0.875F * A, 255),
											   rgba(r * 0.875F * B, g * 0.875F * B, b * 0.875F * B, 255),
											   rgba(r * 0.875F * C, g * 0.875F * C, b * 0.875F * C, 255),
											   rgba(r * 0.875F * D, g * 0.875F * D, b * 0.875F * D, 255),
										   });
						} else {
							tesselator_set_color(tess, rgba(r * 0.875F, g * 0.875F, b * 0.875F, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x, y, z, x, y + 1, z, x + 1, y + 1, z, x + 1, y, z});
						}
					}

					if((z == map_size_z - 1 && map_isair(x, y, 0)) || (z < map_size_z - 1 && map_isair(x, y, z + 1))) {
						if(ao) {
							float A = vertexAO(map_isair(x - 1, y, z + 1), map_isair(x, y - 1, z + 1),
											   map_isair(x - 1, y - 1, z + 1));
							float B = vertexAO(map_isair(x + 1, y, z + 1), map_isair(x, y - 1, z + 1),
											   map_isair(x + 1, y - 1, z + 1));
							float C = vertexAO(map_isair(x + 1, y, z + 1), map_isair(x, y + 1, z + 1),
											   map_isair(x + 1, y + 1, z + 1));
							float D = vertexAO(map_isair(x - 1, y, z + 1), map_isair(x, y + 1, z + 1),
											   map_isair(x - 1, y + 1, z + 1));
							tesselator_add(
								tess, (int16_t[]) {x, y, z + 1, x + 1, y, z + 1, x + 1, y + 1, z + 1, x, y + 1, z + 1},
								(uint32_t[]) {
									rgba(r * 0.625F * A, g * 0.625F * A, b * 0.625F * A, 255),
									rgba(r * 0.625F * B, g * 0.625F * B, b * 0.625F * B, 255),
									rgba(r * 0.625F * C, g * 0.625F * C, b * 0.625F * C, 255),
									rgba(r * 0.625F * D, g * 0.625F * D, b * 0.625F * D, 255),
								});
						} else {
							tesselator_set_color(tess, rgba(r * 0.625F, g * 0.625F, b * 0.625F, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x, y, z + 1, x + 1, y, z + 1, x + 1, y + 1, z + 1, x, y + 1, z + 1});
						}
					}

					if((x == 0 && map_isair(map_size_x - 1, y, z)) || (x > 0 && map_isair(x - 1, y, z))) {
						if(ao) {
							float A = vertexAO(map_isair(x - 1, y - 1, z), map_isair(x - 1, y, z - 1),
											   map_isair(x - 1, y - 1, z - 1));
							float B = vertexAO(map_isair(x - 1, y - 1, z), map_isair(x - 1, y, z + 1),
											   map_isair(x - 1, y - 1, z + 1));
							float C = vertexAO(map_isair(x - 1, y + 1, z), map_isair(x - 1, y, z + 1),
											   map_isair(x - 1, y + 1, z + 1));
							float D = vertexAO(map_isair(x - 1, y + 1, z), map_isair(x - 1, y, z - 1),
											   map_isair(x - 1, y + 1, z - 1));

							tesselator_add(tess, (int16_t[]) {x, y, z, x, y, z + 1, x, y + 1, z + 1, x, y + 1, z},
										   (uint32_t[]) {
											   rgba(r * 0.75F * A, g * 0.75F * A, b * 0.75F * A, 255),
											   rgba(r * 0.75F * B, g * 0.75F * B, b * 0.75F * B, 255),
											   rgba(r * 0.75F * C, g * 0.75F * C, b * 0.75F * C, 255),
											   rgba(r * 0.75F * D, g * 0.75F * D, b * 0.75F * D, 255),
										   });
						} else {
							tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x, y, z, x, y, z + 1, x, y + 1, z + 1, x, y + 1, z});
						}
					}

					if((x == map_size_x - 1 && map_isair(0, y, z)) || (x < map_size_x - 1 && map_isair(x + 1, y, z))) {
						if(ao) {
							float A = vertexAO(map_isair(x + 1, y - 1, z), map_isair(x + 1, y, z - 1),
											   map_isair(x + 1, y - 1, z - 1));
							float B = vertexAO(map_isair(x + 1, y + 1, z), map_isair(x + 1, y, z - 1),
											   map_isair(x + 1, y + 1, z - 1));
							float C = vertexAO(map_isair(x + 1, y + 1, z), map_isair(x + 1, y, z + 1),
											   map_isair(x + 1, y + 1, z + 1));
							float D = vertexAO(map_isair(x + 1, y - 1, z), map_isair(x + 1, y, z + 1),
											   map_isair(x + 1, y - 1, z + 1));

							tesselator_add(
								tess, (int16_t[]) {x + 1, y, z, x + 1, y + 1, z, x + 1, y + 1, z + 1, x + 1, y, z + 1},
								(uint32_t[]) {
									rgba(r * 0.75F * A, g * 0.75F * A, b * 0.75F * A, 255),
									rgba(r * 0.75F * B, g * 0.75F * B, b * 0.75F * B, 255),
									rgba(r * 0.75F * C, g * 0.75F * C, b * 0.75F * C, 255),
									rgba(r * 0.75F * D, g * 0.75F * D, b * 0.75F * D, 255),
								});
						} else {
							tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x + 1, y, z, x + 1, y + 1, z, x + 1, y + 1, z + 1, x + 1, y, z + 1});
						}
					}

					if(y == map_size_y - 1 || map_isair(x, y + 1, z)) {
						if(ao) {
							float A = vertexAO(map_isair(x - 1, y + 1, z), map_isair(x, y + 1, z - 1),
											   map_isair(x - 1, y + 1, z - 1));
							float B = vertexAO(map_isair(x - 1, y + 1, z), map_isair(x, y + 1, z + 1),
											   map_isair(x - 1, y + 1, z + 1));
							float C = vertexAO(map_isair(x + 1, y + 1, z), map_isair(x, y + 1, z + 1),
											   map_isair(x + 1, y + 1, z + 1));
							float D = vertexAO(map_isair(x + 1, y + 1, z), map_isair(x, y + 1, z - 1),
											   map_isair(x + 1, y + 1, z - 1));

							tesselator_add(
								tess, (int16_t[]) {x, y + 1, z, x, y + 1, z + 1, x + 1, y + 1, z + 1, x + 1, y + 1, z},
								(uint32_t[]) {
									rgba(r * A, g * A, b * A, 255),
									rgba(r * B, g * B, b * B, 255),
									rgba(r * C, g * C, b * C, 255),
									rgba(r * D, g * D, b * D, 255),
								});
						} else {
							tesselator_set_color(tess, rgba(r, g, b, 255));
							tesselator_add_simple(
								tess, (int16_t[]) {x, y + 1, z, x, y + 1, z + 1, x + 1, y + 1, z + 1, x + 1, y + 1, z});
						}
					}

					if(y > 0 && map_isair(x, y - 1, z)) {
						if(ao) {
							float A = vertexAO(map_isair(x - 1, y - 1, z), map_isair(x, y - 1, z - 1),
											   map_isair(x - 1, y - 1, z - 1));
							float B = vertexAO(map_isair(x + 1, y - 1, z), map_isair(x, y - 1, z - 1),
											   map_isair(x + 1, y - 1, z - 1));
							float C = vertexAO(map_isair(x + 1, y - 1, z), map_isair(x, y - 1, z + 1),
											   map_isair(x + 1, y - 1, z + 1));
							float D = vertexAO(map_isair(x - 1, y - 1, z), map_isair(x, y - 1, z + 1),
											   map_isair(x - 1, y - 1, z + 1));

							tesselator_add(tess, (int16_t[]) {x, y, z, x + 1, y, z, x + 1, y, z + 1, x, y, z + 1},
										   (uint32_t[]) {
											   rgba(r * 0.5F * A, g * 0.5F * A, b * 0.5F * A, 255),
											   rgba(r * 0.5F * B, g * 0.5F * B, b * 0.5F * B, 255),
											   rgba(r * 0.5F * C, g * 0.5F * C, b * 0.5F * C, 255),
											   rgba(r * 0.5F * D, g * 0.5F * D, b * 0.5F * D, 255),
										   });
						} else {
							tesselator_set_color(tess, rgba(r * 0.5F, g * 0.5F, b * 0.5F, 255));
							tesselator_add_simple(tess,
												  (int16_t[]) {x, y, z, x + 1, y, z, x + 1, y, z + 1, x, y, z + 1});
						}
					}
				}
			}
		}
	}

	(*max_height)++;
}

void chunk_update_all() {
	for(int j = 0; j < chunk_enabled_cores; j++) {
		pthread_mutex_lock(&chunk_workers[j].state_lock);
		if(chunk_workers[j].state == CHUNK_WORKERSTATE_FINISHED) {
			chunk_workers[j].state = CHUNK_WORKERSTATE_IDLE;
			chunks[chunk_workers[j].chunk_id].max_height = chunk_workers[j].max_height;

			tesselator_glx(&chunk_workers[j].tesselator, &chunks[chunk_workers[j].chunk_id].display_list);
			tesselator_free(&chunk_workers[j].tesselator);

			glBindTexture(GL_TEXTURE_2D, texture_minimap.texture_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, chunk_workers[j].chunk_x, chunk_workers[j].chunk_y, CHUNK_SIZE,
							CHUNK_SIZE, GL_RGBA, GL_UNSIGNED_BYTE, chunk_workers[j].minimap_data);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		if(chunk_workers[j].state == CHUNK_WORKERSTATE_IDLE && chunk_geometry_changed_lenght > 0) {
			float closest_dist = FLT_MAX;
			int closest_index = 0;
			for(int k = 0; k < chunk_geometry_changed_lenght; k++) {
				int chunk_x = (chunk_geometry_changed[k] % CHUNKS_PER_DIM) * CHUNK_SIZE;
				int chunk_y = (chunk_geometry_changed[k] / CHUNKS_PER_DIM) * CHUNK_SIZE;
				float l = (chunk_x - camera_x) * (chunk_x - camera_x) + (chunk_y - camera_z) * (chunk_y - camera_z);
				if(l < closest_dist) {
					closest_dist = l;
					closest_index = k;
				}
			}

			int chunk_x = (chunk_geometry_changed[closest_index] % CHUNKS_PER_DIM) * CHUNK_SIZE;
			int chunk_y = (chunk_geometry_changed[closest_index] / CHUNKS_PER_DIM) * CHUNK_SIZE;
			if(!chunks[chunk_geometry_changed[closest_index]].created) {
				glx_displaylist_create(&chunks[chunk_geometry_changed[closest_index]].display_list);
			}
			chunk_workers[j].chunk_id = chunk_geometry_changed[closest_index];
			chunk_workers[j].chunk_x = chunk_x;
			chunk_workers[j].chunk_y = chunk_y;
			chunk_workers[j].state = CHUNK_WORKERSTATE_BUSY;
			chunks[chunk_geometry_changed[closest_index]].last_update = window_time();
			chunks[chunk_geometry_changed[closest_index]].created = 1;

#ifdef OPENGL_ES
			tesselator_create(&chunk_workers[j].tesselator, TESSELATE_TRIANGLES);
#else
			tesselator_create(&chunk_workers[j].tesselator, TESSELATE_QUADS);
#endif

			pthread_cond_signal(&chunk_workers[j].can_work);

			for(int i = closest_index; i < chunk_geometry_changed_lenght - 1; i++) {
				chunk_geometry_changed[i] = chunk_geometry_changed[i + 1];
			}
			chunk_geometry_changed_lenght--;
		}
		pthread_mutex_unlock(&chunk_workers[j].state_lock);
	}
}

void chunk_block_update(int x, int y, int z) {
	int d = (z / CHUNK_SIZE) * CHUNKS_PER_DIM + (x / CHUNK_SIZE);
	for(int k = 0; k < chunk_geometry_changed_lenght; k++) {
		if(chunk_geometry_changed[k] == d) {
			return;
		}
	}
	chunk_geometry_changed[chunk_geometry_changed_lenght++] = d;
}
