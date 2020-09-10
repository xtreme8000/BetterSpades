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
#include <stdbool.h>

#include "glx.h"
#include "tesselator.h"
#include "libvxl.h"

#define CHUNK_SIZE 16
#define CHUNKS_PER_DIM (512 / CHUNK_SIZE)

extern struct chunk {
	struct glx_displaylist display_list;
	int max_height;
	bool updated;
	bool created;
	int x, y;
} chunks[CHUNKS_PER_DIM * CHUNKS_PER_DIM];

#define CHUNK_WORKERS_MAX 16

void chunk_init(void);

void chunk_block_update(int x, int y, int z);
void chunk_update_all(void);
void* chunk_generate(void* data);
void chunk_generate_greedy(int start_x, int start_z, struct tesselator* tess, int* max_height);
void chunk_generate_naive(struct libvxl_block* blocks, int count, uint32_t* solid, struct tesselator* tess,
						  int* max_height, int ao);
void chunk_rebuild_all(void);
void chunk_draw_visible(void);
void chunk_queue_blocks();

#endif
