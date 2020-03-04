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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdint.h>

#include "hashtable.h"
#include "minheap.h"

int int_cmp(void* first_key, void* second_key, size_t key_size) {
	return (*(uint32_t*)first_key) != (*(uint32_t*)second_key);
}

size_t int_hash(void* raw_key, size_t key_size) {
	uint32_t x = *(uint32_t*)raw_key;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

static void nodes_swap(struct minheap* h, int a, int b) {
	struct minheap_block tmp;
	tmp = h->nodes[a];
	h->nodes[a] = h->nodes[b];
	h->nodes[b] = tmp;

	ht_insert(&h->contains, &h->nodes[a].pos, &a);
	ht_insert(&h->contains, &h->nodes[b].pos, &b);
}

void minheap_create(struct minheap* h) {
	h->index = 0;
	h->length = 256;
	h->nodes = malloc(sizeof(struct minheap_block) * h->length);
	ht_setup(&h->contains, sizeof(uint32_t), sizeof(uint32_t), 256);
	h->contains.compare = int_cmp;
	h->contains.hash = int_hash;
}

void minheap_clear(struct minheap* h) {
	ht_clear(&h->contains);
	free(h->nodes);
	h->index = 0;
	h->length = 256;
	h->nodes = malloc(sizeof(struct minheap_block) * h->length);
}

void minheap_destroy(struct minheap* h) {
	free(h->nodes);
	ht_destroy(&h->contains);
}

int minheap_isempty(struct minheap* h) {
	return h->index <= 0;
}

struct minheap_block* minheap_get(struct minheap* h, short x, short y, short z) {
	if(x < 0 || y < 0 || z < 0)
		return NULL;

	uint32_t key = pos_key(x, y, z);
	uint32_t* res = ht_lookup(&h->contains, &key);
	return res ? (h->nodes + *res) : NULL;
}

struct minheap_block minheap_extract(struct minheap* h) {
	struct minheap_block min = h->nodes[0];

	ht_erase(&h->contains, &min.pos);

	h->nodes[0] = h->nodes[--h->index];

	// now heapify at root node
	int k = 0;
	while(1) {
		int smallest = k;
		if(k * 2 + 1 < h->index
		   && pos_keyy(h->nodes[k * 2 + 1].pos)
			   < pos_keyy(h->nodes[smallest].pos)) // does left child exist and is less than parent?
			smallest = k * 2 + 1;
		if(k * 2 + 2 < h->index
		   && pos_keyy(h->nodes[k * 2 + 2].pos)
			   < pos_keyy(h->nodes[smallest].pos)) // does right child exist and is less than parent?
			smallest = k * 2 + 2;
		if(smallest == k) // parent is smallest, finished!
			break;
		nodes_swap(h, k, smallest);
		k = smallest;
	}

	return min;
}

static void minheap_increase(struct minheap* h, struct minheap_block* b, int value) {
	b->pos = pos_key(pos_keyx(b->pos), value, pos_keyy(b->pos));

	int k = b - h->nodes;
	while(1) {
		int smallest = k;
		if(k * 2 + 1 < h->index
		   && pos_keyy(h->nodes[k * 2 + 1].pos)
			   < pos_keyy(h->nodes[smallest].pos)) // does left child exist and is less than parent?
			smallest = k * 2 + 1;
		if(k * 2 + 2 < h->index
		   && pos_keyy(h->nodes[k * 2 + 2].pos)
			   < pos_keyy(h->nodes[smallest].pos)) // does right child exist and is less than parent?
			smallest = k * 2 + 2;
		if(smallest == k) // parent is smallest, finished!
			break;
		nodes_swap(h, k, smallest);
		k = smallest;
	}
}

static void minheap_decrease(struct minheap* h, struct minheap_block* b, int value) {
	b->pos = pos_key(pos_keyx(b->pos), value, pos_keyy(b->pos));

	int k = b - h->nodes;
	while(k > 0) {
		if(pos_keyy(h->nodes[k].pos) < pos_keyy(h->nodes[(k - 1) / 2].pos)) { // is child less than parent?
			nodes_swap(h, k, (k - 1) / 2);
			k = (k - 1) / 2; // continue at parent
		} else {
			break;
		}
	}
}

void minheap_set(struct minheap* h, struct minheap_block* b, int value) {
	if(value > pos_keyy(b->pos))
		minheap_increase(h, b, value);
	else
		minheap_decrease(h, b, value);
}

struct minheap_block* minheap_put(struct minheap* h, struct minheap_block* b) {
	if(h->index >= h->length) { // grow buffer
		h->length *= 2;
		h->nodes = realloc(h->nodes, sizeof(struct minheap_block) * h->length);
	}

	h->nodes[h->index++] = *b; // place new node at end of heap

	int k = h->index - 1;
	while(k > 0) {
		if(pos_keyy(h->nodes[k].pos) < pos_keyy(h->nodes[(k - 1) / 2].pos)) { // is child less than parent?
			nodes_swap(h, k, (k - 1) / 2);
			k = (k - 1) / 2; // continue at parent
		} else {
			break;
		}
	}

	ht_insert(&h->contains, &b->pos, &k);

	return h->nodes + k;
}
