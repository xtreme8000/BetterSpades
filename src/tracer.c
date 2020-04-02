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
#include <math.h>
#include <string.h>

#include "common.h"
#include "matrix.h"
#include "window.h"
#include "tracer.h"
#include "model.h"
#include "camera.h"
#include "texture.h"
#include "config.h"

static struct Tracer* tracers;
static int tracer_insert = 0;
static int tracer_remove = 0;

void tracer_pvelocity(float* o, struct Player* p) {
	o[0] = o[0] * 256.0F / 32.0F + p->physics.velocity.x;
	o[1] = o[1] * 256.0F / 32.0F + p->physics.velocity.y;
	o[2] = o[2] * 256.0F / 32.0F + p->physics.velocity.z;
}

static void tracer_minimap_single(struct Tracer* t, int large, float scalef, float minimap_x, float minimap_y) {
	if(large) {
		if(t->used) {
			float ang = -atan2(t->r.direction.z, t->r.direction.x) - HALFPI;
			texture_draw_rotated(&texture_tracer, minimap_x + t->r.origin.x * scalef,
								 minimap_y - t->r.origin.z * scalef, 15 * scalef, 15 * scalef, ang);
		}
	} else {
		if(t->used) {
			float tracer_x = t->r.origin.x - minimap_x;
			float tracer_y = t->r.origin.z - minimap_y;
			if(tracer_x > 0.0F && tracer_x < 128.0F && tracer_y > 0.0F && tracer_y < 128.0F) {
				float ang = -atan2(t->r.direction.z, t->r.direction.x) - HALFPI;
				texture_draw_rotated(&texture_tracer, settings.window_width - 143 * scalef + tracer_x * scalef,
									 (585 - tracer_y) * scalef, 15 * scalef, 15 * scalef, ang);
			}
		}
	}
}

void tracer_minimap(int large, float scalef, float minimap_x, float minimap_y) {
	if(tracer_remove <= tracer_insert) {
		for(int k = tracer_remove; k < tracer_insert; k++)
			tracer_minimap_single(tracers + k, large, scalef, minimap_x, minimap_y);
	} else {
		for(int k = tracer_remove; k < TRACER_MAX; k++)
			tracer_minimap_single(tracers + k, large, scalef, minimap_x, minimap_y);
		for(int k = 0; k < tracer_insert; k++)
			tracer_minimap_single(tracers + k, large, scalef, minimap_x, minimap_y);
	}
}

void tracer_add(int type, float x, float y, float z, float dx, float dy, float dz) {
	struct Tracer* t = tracers + tracer_insert;
	tracer_insert = (tracer_insert + 1) % TRACER_MAX;
	if(tracer_insert == tracer_remove)
		tracer_remove = (tracer_remove + 1) % TRACER_MAX;

	float spread = 0.0F;
	t->type = type;
	t->x = t->r.origin.x = x + dx / 4.0F;
	t->y = t->r.origin.y = y + dy / 4.0F;
	t->z = t->r.origin.z = z + dz / 4.0F;
	t->r.direction.x = dx;
	t->r.direction.y = dy;
	t->r.direction.z = dz;
	t->created = window_time();

	float len = len3D(dx, dy, dz);
	camera_hit(&t->hit, -1, t->x, t->y, t->z, dx / len, dy / len, dz / len, 128.0F);

	t->used = 1;
}

static void tracer_render_single(struct Tracer* t) {
	struct kv6_t* m[3] = {&model_semi_tracer, &model_smg_tracer, &model_shotgun_tracer};

	if(t->used) {
		matrix_push();
		matrix_translate(t->r.origin.x, t->r.origin.y, t->r.origin.z);
		matrix_pointAt(t->r.direction.x, t->r.direction.y, t->r.direction.z);
		matrix_rotate(90.0F, 0.0F, 1.0F, 0.0F);
		matrix_upload();
		kv6_render(m[t->type], TEAM_SPECTATOR);
		matrix_pop();
	}
}

void tracer_render() {
	if(tracer_remove <= tracer_insert) {
		for(int k = tracer_remove; k < tracer_insert; k++)
			tracer_render_single(tracers + k);
	} else {
		for(int k = tracer_remove; k < TRACER_MAX; k++)
			tracer_render_single(tracers + k);
		for(int k = 0; k < tracer_insert; k++)
			tracer_render_single(tracers + k);
	}
}

static void tracer_update_single(struct Tracer* t, float dt) {
	if(t->used) {
		float len = distance3D(t->x, t->y, t->z, t->r.origin.x, t->r.origin.y, t->r.origin.z);

		// 128.0[m] / 256.0[m/s] = 0.5[s]
		if((t->hit.type != CAMERA_HITTYPE_NONE && len > pow(t->hit.distance, 2)) || window_time() - t->created > 0.5F) {
			if(t->hit.type != CAMERA_HITTYPE_NONE) {
				sound_create(NULL, SOUND_WORLD, &sound_impact, t->r.origin.x, t->r.origin.y, t->r.origin.z);
			}

			t->used = 0;
		} else {
			t->r.origin.x += t->r.direction.x * 32.0F * dt;
			t->r.origin.y += t->r.direction.y * 32.0F * dt;
			t->r.origin.z += t->r.direction.z * 32.0F * dt;
		}
	}
}

void tracer_update(float dt) {
	if(tracer_remove <= tracer_insert) {
		for(int k = tracer_remove; k < tracer_insert; k++)
			tracer_update_single(tracers + k, dt);
	} else {
		for(int k = tracer_remove; k < TRACER_MAX; k++)
			tracer_update_single(tracers + k, dt);
		for(int k = 0; k < tracer_insert; k++)
			tracer_update_single(tracers + k, dt);
	}

	while(!tracers[tracer_remove].used && tracer_remove != tracer_insert) {
		tracer_remove = (tracer_remove + 1) % TRACER_MAX;
	}
}

void tracer_init() {
	tracers = calloc(TRACER_MAX, sizeof(struct Tracer));
	CHECK_ALLOCATION_ERROR(tracers)
}
