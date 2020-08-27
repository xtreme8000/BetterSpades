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

#include "minheap.h"

static void nodes_swap(struct minheap* h, int a, int b) {
	struct minheap_block tmp;
	tmp = h->nodes[a];
	h->nodes[a] = h->nodes[b];
	h->nodes[b] = tmp;
}

void minheap_create(struct minheap* h) {
	h->index = 0;
	h->length = 256;
	h->nodes = malloc(sizeof(struct minheap_block) * h->length);
}

void minheap_clear(struct minheap* h) {
	h->index = 0;
	h->length = 256;
	h->nodes = realloc(h->nodes, sizeof(struct minheap_block) * h->length);
}

void minheap_destroy(struct minheap* h) {
	free(h->nodes);
}

int minheap_isempty(struct minheap* h) {
	return h->index <= 0;
}

struct minheap_block minheap_extract(struct minheap* h) {
	struct minheap_block min = h->nodes[0];

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

	return h->nodes + k;
}
