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

struct chunk chunks[CHUNKS_PER_DIM * CHUNKS_PER_DIM];

struct chunk* chunk_geometry_changed[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
int chunk_geometry_changed_lenght = 0;

struct chunk_worker chunk_workers[CHUNK_WORKERS_MAX];

int chunk_enabled_cores;

void chunk_init() {
	for(int x = 0; x < CHUNKS_PER_DIM; x++) {
		for(int y = 0; y < CHUNKS_PER_DIM; y++) {
			struct chunk* c = chunks + x + y * CHUNKS_PER_DIM;
			c->created = 0;
			c->max_height = 1;
			c->x = x;
			c->y = y;
		}
	}

	chunk_enabled_cores = min(max(window_cpucores() / 2, 1), CHUNK_WORKERS_MAX);
	log_info("%i cores enabled for chunk generation", chunk_enabled_cores);
	for(int k = 0; k < chunk_enabled_cores; k++) {
		chunk_workers[k].state = CHUNK_WORKERSTATE_IDLE;
		pthread_mutex_init(&chunk_workers[k].state_lock, NULL);
		pthread_cond_init(&chunk_workers[k].can_work, NULL);
		pthread_create(&chunk_workers[k].thread, NULL, chunk_generate, &chunk_workers[k]);
	}
}

static int chunk_sort(const void* a, const void* b) {
	struct chunk_render_call* aa = (struct chunk_render_call*)a;
	struct chunk_render_call* bb = (struct chunk_render_call*)b;
	return distance2D(aa->chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2, aa->chunk->y * CHUNK_SIZE + CHUNK_SIZE / 2, camera_x,
					  camera_z)
		- distance2D(bb->chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2, bb->chunk->y * CHUNK_SIZE + CHUNK_SIZE / 2, camera_x,
					 camera_z);
}

void chunk_draw_visible() {
	struct chunk_render_call chunks_draw[CHUNKS_PER_DIM * CHUNKS_PER_DIM * 2];
	int index = 0;

	int overshoot = (settings.render_distance + CHUNK_SIZE - 1) / CHUNK_SIZE + 1;

	// go through all possible chunks and store all in range and view
	for(int y = -overshoot; y < CHUNKS_PER_DIM + overshoot; y++) {
		for(int x = -overshoot; x < CHUNKS_PER_DIM + overshoot; x++) {
			if(distance2D((x + 0.5F) * CHUNK_SIZE, (y + 0.5F) * CHUNK_SIZE, camera_x, camera_z)
			   <= pow(settings.render_distance + 1.414F * CHUNK_SIZE, 2)) {
				uint32_t tmp_x = ((uint32_t)x) % CHUNKS_PER_DIM;
				uint32_t tmp_y = ((uint32_t)y) % CHUNKS_PER_DIM;

				struct chunk* c = chunks + tmp_x + tmp_y * CHUNKS_PER_DIM;

				if(camera_CubeInFrustum((x + 0.5F) * CHUNK_SIZE, 0.0F, (y + 0.5F) * CHUNK_SIZE, CHUNK_SIZE / 2,
										c->max_height))
					chunks_draw[index++] = (struct chunk_render_call) {
						.chunk = c,
						.mirror_x = (x < 0) ? -1 : ((x >= CHUNKS_PER_DIM) ? 1 : 0),
						.mirror_y = (y < 0) ? -1 : ((y >= CHUNKS_PER_DIM) ? 1 : 0),
					};
			}
		}
	}

	// sort all chunks to draw those in front first
	qsort(chunks_draw, index, sizeof(struct chunk_render_call), chunk_sort);

	for(int k = 0; k < index; k++)
		chunk_render(chunks_draw + k);
}

void chunk_render(struct chunk_render_call* c) {
	if(c->chunk->created) {
		matrix_push();
		matrix_translate(c->mirror_x * map_size_x, 0.0F, c->mirror_y * map_size_z);
		matrix_upload();

		// glPolygonMode(GL_FRONT, GL_LINE);

		glx_displaylist_draw(&c->chunk->display_list, GLX_DISPLAYLIST_NORMAL);

		// glPolygonMode(GL_FRONT, GL_FILL);

		matrix_pop();
	}
}

static __attribute__((always_inline)) inline uint32_t solid_array_isair(uint32_t* array, uint32_t x, uint32_t y,
																		uint32_t z) {
	if(y < 0)
		return 0;
	if(y >= map_size_y)
		return 1;

	size_t offset = (map_size_y - 1 - y) + ((x % map_size_x) + (z % map_size_z) * map_size_x) * map_size_y;

	return !(array[offset / (sizeof(uint32_t) * 8)] & (1 << (offset % (sizeof(uint32_t) * 8))));
}

static __attribute__((always_inline)) inline float solid_sunblock(uint32_t* array, uint32_t x, uint32_t y, uint32_t z) {
	int dec = 18;
	int i = 127;

	while(dec && y < map_size_y) {
		if(!solid_array_isair(array, x, ++y, --z))
			i -= dec;
		dec -= 2;
	}
	return (float)i / 127.0F;
}

void* chunk_generate(void* data) {
	pthread_detach(pthread_self());

	struct chunk_worker* worker = (struct chunk_worker*)data;

	int first_start = 1;

	while(1) {
		pthread_mutex_lock(&worker->state_lock);
		if(!first_start) {
			worker->state = CHUNK_WORKERSTATE_FINISHED;
		}

		while(worker->state != CHUNK_WORKERSTATE_BUSY)
			pthread_cond_wait(&worker->can_work, &worker->state_lock);
		pthread_mutex_unlock(&worker->state_lock);

		first_start = 0;

		/*if(settings.greedy_meshing)
			chunk_generate_greedy(worker->chunk_x, worker->chunk_y, &worker->tesselator, &worker->max_height);
		else*/
		chunk_generate_naive(worker->blocks, worker->blocks_count, worker->blocks_solid, &worker->tesselator,
							 &worker->max_height, settings.ambient_occlusion);

		// use the fact that libvxl orders libvxl_blocks by top-down coordinate first in its data structure
		uint32_t last_position = 0;
		for(int k = worker->blocks_count - 1; k >= 0; k--) {
			struct libvxl_block* blk = worker->blocks + k;

			if(blk->position != last_position || k == worker->blocks_count - 1) {
				last_position = blk->position;

				int x = key_getx(blk->position);
				int z = key_gety(blk->position);

				uint32_t* out = worker->minimap_data + (x - worker->chunk_x + (z - worker->chunk_y) * CHUNK_SIZE);

				if((x % 64) > 0 && (z % 64) > 0) {
					*out = rgb2bgr(blk->color) | 0xFF000000;
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
							tesselator_addi_simple(
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
							tesselator_addi_simple(tess,
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
							tesselator_addi_simple(
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
							tesselator_addi_simple(tess,
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
							tesselator_addi_simple(tess,
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
							tesselator_addi_simple(
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

void chunk_generate_naive(struct libvxl_block* blocks, int count, uint32_t* solid, struct tesselator* tess,
						  int* max_height, int ao) {
	*max_height = 0;

	for(int k = 0; k < count; k++) {
		struct libvxl_block* blk = blocks + k;

		int x = key_getx(blk->position);
		int y = map_size_y - 1 - key_getz(blk->position);
		int z = key_gety(blk->position);

		if(*max_height < y) {
			*max_height = y;
		}

		uint32_t col = blk->color;
		int r = blue(col);
		int g = green(col);
		int b = red(col);

		float shade = solid_sunblock(solid, x, y, z);
		r *= shade;
		g *= shade;
		b *= shade;

		if(solid_array_isair(solid, x, y, z - 1)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x - 1, y, z - 1), solid_array_isair(solid, x, y - 1, z - 1),
								   solid_array_isair(solid, x - 1, y - 1, z - 1));
				float B = vertexAO(solid_array_isair(solid, x - 1, y, z - 1), solid_array_isair(solid, x, y + 1, z - 1),
								   solid_array_isair(solid, x - 1, y + 1, z - 1));
				float C = vertexAO(solid_array_isair(solid, x + 1, y, z - 1), solid_array_isair(solid, x, y + 1, z - 1),
								   solid_array_isair(solid, x + 1, y + 1, z - 1));
				float D = vertexAO(solid_array_isair(solid, x + 1, y, z - 1), solid_array_isair(solid, x, y - 1, z - 1),
								   solid_array_isair(solid, x + 1, y - 1, z - 1));

				tesselator_addi(tess, (int16_t[]) {x, y, z, x, y + 1, z, x + 1, y + 1, z, x + 1, y, z},
								(uint32_t[]) {
									rgba(r * 0.875F * A, g * 0.875F * A, b * 0.875F * A, 255),
									rgba(r * 0.875F * B, g * 0.875F * B, b * 0.875F * B, 255),
									rgba(r * 0.875F * C, g * 0.875F * C, b * 0.875F * C, 255),
									rgba(r * 0.875F * D, g * 0.875F * D, b * 0.875F * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r * 0.875F, g * 0.875F, b * 0.875F, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_Z_N, x, y, z);
			}
		}

		if(solid_array_isair(solid, x, y, z + 1)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x - 1, y, z + 1), solid_array_isair(solid, x, y - 1, z + 1),
								   solid_array_isair(solid, x - 1, y - 1, z + 1));
				float B = vertexAO(solid_array_isair(solid, x + 1, y, z + 1), solid_array_isair(solid, x, y - 1, z + 1),
								   solid_array_isair(solid, x + 1, y - 1, z + 1));
				float C = vertexAO(solid_array_isair(solid, x + 1, y, z + 1), solid_array_isair(solid, x, y + 1, z + 1),
								   solid_array_isair(solid, x + 1, y + 1, z + 1));
				float D = vertexAO(solid_array_isair(solid, x - 1, y, z + 1), solid_array_isair(solid, x, y + 1, z + 1),
								   solid_array_isair(solid, x - 1, y + 1, z + 1));
				tesselator_addi(tess, (int16_t[]) {x, y, z + 1, x + 1, y, z + 1, x + 1, y + 1, z + 1, x, y + 1, z + 1},
								(uint32_t[]) {
									rgba(r * 0.625F * A, g * 0.625F * A, b * 0.625F * A, 255),
									rgba(r * 0.625F * B, g * 0.625F * B, b * 0.625F * B, 255),
									rgba(r * 0.625F * C, g * 0.625F * C, b * 0.625F * C, 255),
									rgba(r * 0.625F * D, g * 0.625F * D, b * 0.625F * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r * 0.625F, g * 0.625F, b * 0.625F, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_Z_P, x, y, z);
			}
		}

		if(solid_array_isair(solid, x - 1, y, z)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x - 1, y - 1, z), solid_array_isair(solid, x - 1, y, z - 1),
								   solid_array_isair(solid, x - 1, y - 1, z - 1));
				float B = vertexAO(solid_array_isair(solid, x - 1, y - 1, z), solid_array_isair(solid, x - 1, y, z + 1),
								   solid_array_isair(solid, x - 1, y - 1, z + 1));
				float C = vertexAO(solid_array_isair(solid, x - 1, y + 1, z), solid_array_isair(solid, x - 1, y, z + 1),
								   solid_array_isair(solid, x - 1, y + 1, z + 1));
				float D = vertexAO(solid_array_isair(solid, x - 1, y + 1, z), solid_array_isair(solid, x - 1, y, z - 1),
								   solid_array_isair(solid, x - 1, y + 1, z - 1));

				tesselator_addi(tess, (int16_t[]) {x, y, z, x, y, z + 1, x, y + 1, z + 1, x, y + 1, z},
								(uint32_t[]) {
									rgba(r * 0.75F * A, g * 0.75F * A, b * 0.75F * A, 255),
									rgba(r * 0.75F * B, g * 0.75F * B, b * 0.75F * B, 255),
									rgba(r * 0.75F * C, g * 0.75F * C, b * 0.75F * C, 255),
									rgba(r * 0.75F * D, g * 0.75F * D, b * 0.75F * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_X_N, x, y, z);
			}
		}

		if(solid_array_isair(solid, x + 1, y, z)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x + 1, y - 1, z), solid_array_isair(solid, x + 1, y, z - 1),
								   solid_array_isair(solid, x + 1, y - 1, z - 1));
				float B = vertexAO(solid_array_isair(solid, x + 1, y + 1, z), solid_array_isair(solid, x + 1, y, z - 1),
								   solid_array_isair(solid, x + 1, y + 1, z - 1));
				float C = vertexAO(solid_array_isair(solid, x + 1, y + 1, z), solid_array_isair(solid, x + 1, y, z + 1),
								   solid_array_isair(solid, x + 1, y + 1, z + 1));
				float D = vertexAO(solid_array_isair(solid, x + 1, y - 1, z), solid_array_isair(solid, x + 1, y, z + 1),
								   solid_array_isair(solid, x + 1, y - 1, z + 1));

				tesselator_addi(tess, (int16_t[]) {x + 1, y, z, x + 1, y + 1, z, x + 1, y + 1, z + 1, x + 1, y, z + 1},
								(uint32_t[]) {
									rgba(r * 0.75F * A, g * 0.75F * A, b * 0.75F * A, 255),
									rgba(r * 0.75F * B, g * 0.75F * B, b * 0.75F * B, 255),
									rgba(r * 0.75F * C, g * 0.75F * C, b * 0.75F * C, 255),
									rgba(r * 0.75F * D, g * 0.75F * D, b * 0.75F * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r * 0.75F, g * 0.75F, b * 0.75F, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_X_P, x, y, z);
			}
		}

		if(y == map_size_y - 1 || solid_array_isair(solid, x, y + 1, z)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x - 1, y + 1, z), solid_array_isair(solid, x, y + 1, z - 1),
								   solid_array_isair(solid, x - 1, y + 1, z - 1));
				float B = vertexAO(solid_array_isair(solid, x - 1, y + 1, z), solid_array_isair(solid, x, y + 1, z + 1),
								   solid_array_isair(solid, x - 1, y + 1, z + 1));
				float C = vertexAO(solid_array_isair(solid, x + 1, y + 1, z), solid_array_isair(solid, x, y + 1, z + 1),
								   solid_array_isair(solid, x + 1, y + 1, z + 1));
				float D = vertexAO(solid_array_isair(solid, x + 1, y + 1, z), solid_array_isair(solid, x, y + 1, z - 1),
								   solid_array_isair(solid, x + 1, y + 1, z - 1));

				tesselator_addi(tess, (int16_t[]) {x, y + 1, z, x, y + 1, z + 1, x + 1, y + 1, z + 1, x + 1, y + 1, z},
								(uint32_t[]) {
									rgba(r * A, g * A, b * A, 255),
									rgba(r * B, g * B, b * B, 255),
									rgba(r * C, g * C, b * C, 255),
									rgba(r * D, g * D, b * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r, g, b, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_Y_P, x, y, z);
			}
		}

		if(y > 0 && solid_array_isair(solid, x, y - 1, z)) {
			if(ao) {
				float A = vertexAO(solid_array_isair(solid, x - 1, y - 1, z), solid_array_isair(solid, x, y - 1, z - 1),
								   solid_array_isair(solid, x - 1, y - 1, z - 1));
				float B = vertexAO(solid_array_isair(solid, x + 1, y - 1, z), solid_array_isair(solid, x, y - 1, z - 1),
								   solid_array_isair(solid, x + 1, y - 1, z - 1));
				float C = vertexAO(solid_array_isair(solid, x + 1, y - 1, z), solid_array_isair(solid, x, y - 1, z + 1),
								   solid_array_isair(solid, x + 1, y - 1, z + 1));
				float D = vertexAO(solid_array_isair(solid, x - 1, y - 1, z), solid_array_isair(solid, x, y - 1, z + 1),
								   solid_array_isair(solid, x - 1, y - 1, z + 1));

				tesselator_addi(tess, (int16_t[]) {x, y, z, x + 1, y, z, x + 1, y, z + 1, x, y, z + 1},
								(uint32_t[]) {
									rgba(r * 0.5F * A, g * 0.5F * A, b * 0.5F * A, 255),
									rgba(r * 0.5F * B, g * 0.5F * B, b * 0.5F * B, 255),
									rgba(r * 0.5F * C, g * 0.5F * C, b * 0.5F * C, 255),
									rgba(r * 0.5F * D, g * 0.5F * D, b * 0.5F * D, 255),
								},
								NULL);
			} else {
				tesselator_set_color(tess, rgba(r * 0.5F, g * 0.5F, b * 0.5F, 255));
				tesselator_addi_cube_face(tess, CUBE_FACE_Y_N, x, y, z);
			}
		}
	}

	(*max_height)++;
}

void chunk_update_all() {
	struct chunk* in_progress[chunk_enabled_cores];

	// collect all chunks that are currently processed
	for(int j = 0; j < chunk_enabled_cores; j++) {
		struct chunk_worker* worker = chunk_workers + j;
		pthread_mutex_lock(&worker->state_lock);
		in_progress[j] = (worker->state != CHUNK_WORKERSTATE_IDLE) ? worker->chunk : NULL;
		pthread_mutex_unlock(&worker->state_lock);
	}

	for(int j = 0; j < chunk_enabled_cores; j++) {
		struct chunk_worker* worker = chunk_workers + j;

		pthread_mutex_lock(&worker->state_lock);

		if(worker->state == CHUNK_WORKERSTATE_FINISHED) {
			// chunk of this worker is no longer in progress
			in_progress[j] = NULL;

			worker->state = CHUNK_WORKERSTATE_IDLE;
			worker->chunk->max_height = worker->max_height;

			free(worker->blocks);
			free(worker->blocks_solid);

			tesselator_glx(&worker->tesselator, &worker->chunk->display_list);
			tesselator_free(&worker->tesselator);

			glBindTexture(GL_TEXTURE_2D, texture_minimap.texture_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, worker->chunk_x, worker->chunk_y, CHUNK_SIZE, CHUNK_SIZE, GL_RGBA,
							GL_UNSIGNED_BYTE, worker->minimap_data);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if(worker->state == CHUNK_WORKERSTATE_IDLE && chunk_geometry_changed_lenght > 0) {
			float closest_dist = FLT_MAX;
			int closest_index = -1;
			for(int k = 0; k < chunk_geometry_changed_lenght; k++) {
				int can_take = 1;

				for(int i = 0; i < chunk_enabled_cores; i++) {
					if(in_progress[i] && in_progress[i] == chunk_geometry_changed[k]) {
						can_take = 0;
						break;
					}
				}

				if(can_take) {
					float l = distance2D(chunk_geometry_changed[k]->x * CHUNK_SIZE,
										 chunk_geometry_changed[k]->y * CHUNK_SIZE, camera_x, camera_z);
					if(l < closest_dist) {
						closest_dist = l;
						closest_index = k;
					}
				}
			}

			if(closest_index >= 0) {
				struct chunk* c = chunk_geometry_changed[closest_index];

				in_progress[j] = c;

				chunk_geometry_changed[closest_index] = chunk_geometry_changed[--chunk_geometry_changed_lenght];

				if(!c->created) {
					glx_displaylist_create(&c->display_list, true, false);
					c->created = 1;
				}

				c->last_update = window_time();
				worker->chunk = c;
				worker->chunk_x = c->x * CHUNK_SIZE;
				worker->chunk_y = c->y * CHUNK_SIZE;
				worker->state = CHUNK_WORKERSTATE_BUSY;
				worker->blocks = map_copy_blocks(c->x, c->y, &worker->blocks_count);
				worker->blocks_solid = map_copy_solids();

				tesselator_create(&worker->tesselator, VERTEX_INT, 0);

				pthread_cond_signal(&worker->can_work);
			}
		}

		pthread_mutex_unlock(&worker->state_lock);
	}
}

void chunk_rebuild_all() {
	for(int d = 0; d < CHUNKS_PER_DIM * CHUNKS_PER_DIM; d++) {
		chunk_geometry_changed[d] = chunks + d;
	}

	chunk_geometry_changed_lenght = CHUNKS_PER_DIM * CHUNKS_PER_DIM;
}

void chunk_block_update(int x, int y, int z) {
	struct chunk* c = chunks + (x / CHUNK_SIZE) + (z / CHUNK_SIZE) * CHUNKS_PER_DIM;

	for(int k = 0; k < chunk_geometry_changed_lenght; k++) {
		if(chunk_geometry_changed[k] == c) {
			return;
		}
	}

	chunk_geometry_changed[chunk_geometry_changed_lenght++] = c;
}
