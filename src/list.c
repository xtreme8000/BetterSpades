#include "common.h"

void list_create(struct list* l, int element_size) {
    l->data = NULL;
    l->elements = 0;
    l->element_size = element_size;
    l->mem_size = 0;
}

void* list_get(struct list* l, int i) {
    return l->data+i*l->element_size;
}

void* list_add(struct list* l, void* e) {
    if((l->elements+1)*l->element_size>l->mem_size) {
        l->mem_size += l->element_size*64;
        l->data = realloc(l->data,l->mem_size);
    }
    memcpy(l->data+l->elements*l->element_size,e,l->element_size);
    return l->data+(l->elements++)*l->element_size;
}

void list_remove(struct list* l, int i) {
    memmove(list_get(l,i),list_get(l,i+1),(l->elements-(i+1))*l->element_size);
    l->elements--;
    if(l->mem_size-64*l->element_size>0 && (l->elements+1)*l->element_size<l->mem_size-96*l->element_size) {
        l->mem_size -= l->element_size*64;
        l->data = realloc(l->data,l->mem_size);
    }
}

void list_clear(struct list* l) {
    l->mem_size = l->element_size*64;
    l->data = realloc(l->data,l->mem_size);
    l->elements = 0;
    l->mem_size = 0;
}

int list_size(struct list* l) {
    return l->elements;
}
