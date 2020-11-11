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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

struct channel {
	size_t object_size;
	size_t initial_length;
	size_t length;
	size_t count;
	size_t loc_insert;
	size_t loc_remove;
	void* queue;
	pthread_mutex_t lock;
	pthread_cond_t signal;
};

bool channel_create(struct channel* ch, size_t object_size, size_t length);

size_t channel_size(struct channel* ch);

void channel_destroy(struct channel* ch);

void channel_put(struct channel* ch, void* object);

void channel_await(struct channel* ch, void* object);

void channel_clear(struct channel* ch);

#endif
