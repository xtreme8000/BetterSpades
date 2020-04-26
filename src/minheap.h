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

#ifndef MINHEAP_H
#define MINHEAP_H

#define pos_key(x, y, z) (((z) << 20) | ((x) << 8) | (y))
#define pos_keyx(key) (((key) >> 8) & 0xFFF)
#define pos_keyy(key) ((key)&0xFF)
#define pos_keyz(key) (((key) >> 20) & 0xFFF)

struct path_vec {
	short x, y, z;
};

struct minheap_block {
	uint32_t pos;
};

struct minheap {
	int index;
	int length;
	struct minheap_block* nodes;
};

int int_cmp(void* first_key, void* second_key, size_t key_size);
size_t int_hash(void* raw_key, size_t key_size);

void minheap_create(struct minheap* h);
void minheap_clear(struct minheap* h);
void minheap_destroy(struct minheap* h);
int minheap_isempty(struct minheap* h);
struct minheap_block minheap_extract(struct minheap* h);
void minheap_set(struct minheap* h, struct minheap_block* b, int value);
struct minheap_block* minheap_put(struct minheap* h, struct minheap_block* b);

#endif
