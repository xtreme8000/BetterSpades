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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"

bool channel_create(struct channel* ch, size_t object_size, size_t length) {
	assert(ch != NULL && object_size > 0 && length > 0);

	ch->object_size = object_size;
	ch->length = length;
	ch->count = 0;
	ch->queue = malloc(object_size * length);
	ch->loc_insert = 0;
	ch->loc_remove = 0;

	if(!ch->queue)
		return false;

	if(pthread_mutex_init(&ch->lock, NULL)) {
		free(ch->queue);
		return false;
	}

	if(pthread_cond_init(&ch->signal, NULL)) {
		free(ch->queue);
		pthread_mutex_destroy(ch->lock);
		return false;
	}

	return true;
}

size_t channel_size(struct channel* ch) {
	assert(ch != NULL);

	pthread_mutex_lock(&ch->lock);
	size_t res = ch->count;
	pthread_mutex_unlock(&ch->lock);
	return res;
}

void channel_destroy(struct channel* ch) {
	assert(ch != NULL);

	free(ch->queue);
	pthread_cond_destroy(&ch->signal);
	pthread_mutex_destroy(&ch->lock);
}

void channel_put(struct channel* ch, void* object) {
	assert(ch != NULL && object != NULL);

	pthread_mutex_lock(&ch->lock);
	while(ch->count >= ch->length)
		pthread_cond_wait(&ch->signal, &ch->lock);

	memcpy((uint8_t*)ch->queue + ch->loc_insert * ch->object_size, object, ch->object_size);
	ch->loc_insert = (ch->loc_insert + 1) % ch->length;
	ch->count++;

	pthread_cond_signal(&ch->signal);
	pthread_mutex_unlock(&ch->lock);
}

void channel_await(struct channel* ch, void* object) {
	assert(ch != NULL && object != NULL);

	pthread_mutex_lock(&ch->lock);
	while(!ch->count)
		pthread_cond_wait(&ch->signal, &ch->lock);

	memcpy(object, (uint8_t*)ch->queue + ch->loc_remove * ch->object_size, ch->object_size);
	ch->loc_remove = (ch->loc_remove + 1) % ch->length;
	ch->count--;

	pthread_cond_signal(&ch->signal);
	pthread_mutex_unlock(&ch->lock);
}

void channel_clear(struct channel* ch) {
	assert(ch != NULL);

	pthread_mutex_lock(&ch->lock);
	ch->count = 0;
	ch->loc_remove = 0;
	ch->loc_insert = 0;

	pthread_cond_signal(&ch->signal);
	pthread_mutex_unlock(&ch->lock);
}
