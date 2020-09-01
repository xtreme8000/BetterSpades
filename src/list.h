#ifndef LIST_H
#define LIST_H

struct list {
	void* data;
	size_t elements, element_size, mem_size;
};

enum list_traverse_direction {
	LIST_TRAVERSE_FORWARD,
	LIST_TRAVERSE_BACKWARD,
};

int list_created(struct list* l);
void list_create(struct list* l, size_t element_size);
void list_free(struct list* l);
void list_sort(struct list* l, int (*cmp)(const void* obj, const void* ref));
void* list_find(struct list* l, void* ref, enum list_traverse_direction dir, int (*cmp)(void* a, void* b));
void* list_get(struct list* l, size_t i);
void* list_add(struct list* l, void* e);
void list_remove(struct list* l, size_t i);
void list_clear(struct list* l);
int list_size(struct list* l);

#endif
