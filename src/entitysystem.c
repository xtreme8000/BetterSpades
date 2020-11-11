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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "entitysystem.h"

void entitysys_create(struct entity_system* es, size_t object_size, size_t initial_size) {
	assert(es != NULL && object_size > 0 && initial_size > 0);

	es->buffer = malloc(object_size * initial_size);
	es->count = 0;
	es->object_size = object_size;
	es->length = initial_size;

	pthread_mutex_init(&es->lock, NULL);
}

void entitysys_iterate(struct entity_system* es, void* user, bool (*callback)(void* object, void* user)) {
	assert(es != NULL && callback != NULL);

	pthread_mutex_lock(&es->lock);

	uint8_t* obj = es->buffer;
	for(size_t k = 0; k < es->count; k++, obj += es->object_size) {
		if(callback(obj, user)) {
			if(es->count > 1)
				memcpy(obj, (uint8_t*)es->buffer + es->object_size * (es->count - 1), es->object_size);

			es->count--;
		}
	}

	pthread_mutex_unlock(&es->lock);
}

void entitysys_add(struct entity_system* es, void* object) {
	assert(es != NULL && object != NULL);

	pthread_mutex_lock(&es->lock);

	if(es->count >= es->length) {
		es->length *= 2;
		es->buffer = realloc(es->buffer, es->object_size * es->length);
	}

	memcpy((uint8_t*)es->buffer + es->object_size * (es->count++), object, es->object_size);

	pthread_mutex_unlock(&es->lock);
}
