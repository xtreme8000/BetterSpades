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

#ifndef ENTITY_SYSTEM_H
#define ENTITY_SYSTEM_H

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

struct entity_system {
	void* buffer;
	size_t count;
	size_t length;
	size_t object_size;
	pthread_mutex_t lock;
};

void entitysys_create(struct entity_system* es, size_t object_size, size_t initial_size);

void entitysys_add(struct entity_system* es, void* object);

void entitysys_iterate(struct entity_system* es, void* user, bool (*callback)(void* object, void* user));

#endif
