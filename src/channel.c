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

static void channel_sanity_checks(struct channel* ch) {
	assert(ch != NULL);

	assert(ch->queue != NULL);
	assert(ch->object_size > 0);
	assert(ch->loc_remove < ch->length);
	assert(ch->loc_insert < ch->length);
	assert(ch->count <= ch->length);
	assert(ch->length >= ch->initial_length);

	if(ch->loc_remove < ch->loc_insert) {
		assert(ch->loc_insert - ch->loc_remove == ch->count);
	} else if(ch->loc_insert < ch->loc_remove) {
		assert(ch->loc_insert + (ch->length - ch->loc_remove) == ch->count);
	} else {
		assert(ch->count == ch->length || ch->count == 0);
	}
}

bool channel_create(struct channel* ch, size_t object_size, size_t length) {
	assert(ch != NULL && object_size > 0 && length > 0);

	ch->object_size = object_size;
	ch->initial_length = length;
	ch->length = ch->initial_length;
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
		pthread_mutex_destroy(&ch->lock);
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

// call once buffer is full
static void channel_grow(struct channel* ch) {
	assert(ch != NULL);

	size_t length = ch->length * 2;

	ch->queue = realloc(ch->queue, ch->object_size * length);

	if(ch->loc_insert <= ch->loc_remove) {
		size_t object_count = ch->length - ch->loc_remove;
		size_t new_remove_loc = length - object_count;

		memcpy((uint8_t*)ch->queue + new_remove_loc * ch->object_size,
			   (uint8_t*)ch->queue + ch->loc_remove * ch->object_size, object_count * ch->object_size);

		ch->loc_remove = new_remove_loc;
	}

	ch->length = length;

	channel_sanity_checks(ch);
}

// call once only 25% of elements left, we will halve the buffer, so its filled exactly by 50%
static void channel_shrink(struct channel* ch) {
	assert(ch != NULL);

	size_t length = ch->length / 2;

	/* Is the run of elements ending in the to be removed section? Note, that there are only 25% of elements left, so
	 * the run can't wrap around to the left. If yes, move all elements to the start. */
	if(ch->loc_insert > length && ch->loc_remove < ch->loc_insert) {
		memmove(ch->queue, (uint8_t*)ch->queue + ch->loc_remove * ch->object_size, ch->count * ch->object_size);
		ch->loc_remove = 0;
		ch->loc_insert = ch->count;

		/* Is the run of elements wrapping at the buffer's borders? If yes, copy the right part to the new ending
		 * (middle). We won't override existing data, since all elements will only occupy 50% once shrinked. */
	} else if(ch->loc_insert < ch->loc_remove) {
		size_t object_count = ch->length - ch->loc_remove;
		size_t new_remove_loc = length - object_count;

		memcpy((uint8_t*)ch->queue + new_remove_loc * ch->object_size,
			   (uint8_t*)ch->queue + ch->loc_remove * ch->object_size, object_count * ch->object_size);

		ch->loc_remove = new_remove_loc;
	}

	ch->length = length;
	ch->queue = realloc(ch->queue, ch->object_size * ch->length);
	ch->loc_remove %= ch->length;
	ch->loc_insert %= ch->length;

	channel_sanity_checks(ch);
}

void channel_put(struct channel* ch, void* object) {
	assert(ch != NULL && object != NULL);

	pthread_mutex_lock(&ch->lock);
	channel_sanity_checks(ch);

	if(ch->count >= ch->length)
		channel_grow(ch);

	memcpy((uint8_t*)ch->queue + ch->loc_insert * ch->object_size, object, ch->object_size);
	ch->loc_insert = (ch->loc_insert + 1) % ch->length;
	ch->count++;

	channel_sanity_checks(ch);

	pthread_cond_signal(&ch->signal);
	pthread_mutex_unlock(&ch->lock);
}

void channel_await(struct channel* ch, void* object) {
	assert(ch != NULL && object != NULL);

	pthread_mutex_lock(&ch->lock);
	channel_sanity_checks(ch);

	while(!ch->count)
		pthread_cond_wait(&ch->signal, &ch->lock);

	memcpy(object, (uint8_t*)ch->queue + ch->loc_remove * ch->object_size, ch->object_size);
	ch->loc_remove = (ch->loc_remove + 1) % ch->length;
	ch->count--;

	if(ch->length / 2 >= ch->initial_length && ch->count <= ch->length / 4)
		channel_shrink(ch);

	channel_sanity_checks(ch);

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
