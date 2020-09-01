#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "list.h"

int list_created(struct list* l) {
	assert(l != NULL);

	return l->element_size > 0;
}

void list_create(struct list* l, size_t element_size) {
	assert(l != NULL && element_size > 0);

	l->data = NULL;
	l->elements = 0;
	l->element_size = element_size;
	l->mem_size = 0;
}

void list_free(struct list* l) {
	assert(l != NULL);

	if(l->data) {
		free(l->data);
		l->data = NULL;
	}
}

void* list_find(struct list* l, void* ref, enum list_traverse_direction dir, int (*cmp)(void* obj, void* ref)) {
	assert(l != NULL && cmp != NULL);

	switch(dir) {
		case LIST_TRAVERSE_FORWARD:
			for(size_t k = 0; k < l->elements; k++) {
				void* obj = l->data + l->element_size * k;

				if(cmp(obj, ref))
					return obj;
			}
			break;
		case LIST_TRAVERSE_BACKWARD:
			if(!l->elements)
				break;

			size_t k = l->elements - 1;

			do {
				void* obj = l->data + l->element_size * k;

				if(cmp(obj, ref))
					return obj;
			} while((k--) > 0);

			break;
	}

	return NULL;
}

void list_sort(struct list* l, int (*cmp)(const void* a, const void* b)) {
	assert(l != NULL && cmp != NULL);

	qsort(l->data, l->elements, l->element_size, cmp);
}

void* list_get(struct list* l, size_t i) {
	assert(l != NULL && i < l->elements);

	return l->data + i * l->element_size;
}

void* list_add(struct list* l, void* e) {
	assert(l != NULL);

	if((l->elements + 1) * l->element_size > l->mem_size) {
		l->mem_size += l->element_size * 64;
		l->data = realloc(l->data, l->mem_size);
		CHECK_ALLOCATION_ERROR(l->data)
	}

	if(e)
		memcpy(l->data + l->elements * l->element_size, e, l->element_size);
	else
		memset(l->data + l->elements * l->element_size, 0, l->element_size);

	return l->data + (l->elements++) * l->element_size;
}

void list_remove(struct list* l, size_t i) {
	assert(l != NULL && i < l->elements);

	if(i < l->elements - 1)
		memmove(list_get(l, i), list_get(l, i + 1), (l->elements - (i + 1)) * l->element_size);

	l->elements--;

	if(l->mem_size - 64 * l->element_size > 0
	   && (l->elements + 1) * l->element_size < l->mem_size - 96 * l->element_size) {
		l->mem_size -= l->element_size * 64;
		l->data = realloc(l->data, l->mem_size);
		CHECK_ALLOCATION_ERROR(l->data)
	}
}

void list_clear(struct list* l) {
	assert(l != NULL);

	void* new = realloc(l->data, l->element_size * 64);

	if(new) {
		l->data = new;
		l->mem_size = l->element_size * 64;
	}

	l->elements = 0;
}

int list_size(struct list* l) {
	assert(l != NULL);

	return l->elements;
}
