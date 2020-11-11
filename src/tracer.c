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
#include "sound.h"
#include "entitysystem.h"

struct entity_system tracers;

void tracer_pvelocity(float* o, struct Player* p) {
	o[0] = o[0] * 256.0F / 32.0F + p->physics.velocity.x;
	o[1] = o[1] * 256.0F / 32.0F + p->physics.velocity.y;
	o[2] = o[2] * 256.0F / 32.0F + p->physics.velocity.z;
}

struct tracer_minimap_info {
	int large;
	float scalef;
	float minimap_x;
	float minimap_y;
};

static bool tracer_minimap_single(void* obj, void* user) {
	struct Tracer* t = (struct Tracer*)obj;
	struct tracer_minimap_info* info = (struct tracer_minimap_info*)user;

	if(info->large) {
		float ang = -atan2(t->r.direction.z, t->r.direction.x) - HALFPI;
		texture_draw_rotated(&texture_tracer, info->minimap_x + t->r.origin.x * info->scalef,
							 info->minimap_y - t->r.origin.z * info->scalef, 15 * info->scalef, 15 * info->scalef, ang);
	} else {
		float tracer_x = t->r.origin.x - info->minimap_x;
		float tracer_y = t->r.origin.z - info->minimap_y;
		if(tracer_x > 0.0F && tracer_x < 128.0F && tracer_y > 0.0F && tracer_y < 128.0F) {
			float ang = -atan2(t->r.direction.z, t->r.direction.x) - HALFPI;
			texture_draw_rotated(&texture_tracer, settings.window_width - 143 * info->scalef + tracer_x * info->scalef,
								 (585 - tracer_y) * info->scalef, 15 * info->scalef, 15 * info->scalef, ang);
		}
	}

	return false;
}

void tracer_minimap(int large, float scalef, float minimap_x, float minimap_y) {
	entitysys_iterate(&tracers,
					  &(struct tracer_minimap_info) {
						  .large = large,
						  .scalef = scalef,
						  .minimap_x = minimap_x,
						  .minimap_y = minimap_y,
					  },
					  tracer_minimap_single);
}

void tracer_add(int type, float x, float y, float z, float dx, float dy, float dz) {
	struct Tracer t = (struct Tracer) {
		.type = type,
		.x = t.r.origin.x = x + dx / 4.0F,
		.y = t.r.origin.y = y + dy / 4.0F,
		.z = t.r.origin.z = z + dz / 4.0F,
		.r.direction.x = dx,
		.r.direction.y = dy,
		.r.direction.z = dz,
		.created = window_time(),
	};

	float len = len3D(dx, dy, dz);
	camera_hit(&t.hit, -1, t.x, t.y, t.z, dx / len, dy / len, dz / len, 128.0F);

	entitysys_add(&tracers, &t);
}

static bool tracer_render_single(void* obj, void* user) {
	struct Tracer* t = (struct Tracer*)obj;

	matrix_push();
	matrix_translate(t->r.origin.x, t->r.origin.y, t->r.origin.z);
	matrix_pointAt(t->r.direction.x, t->r.direction.y, t->r.direction.z);
	matrix_rotate(90.0F, 0.0F, 1.0F, 0.0F);
	matrix_upload();
	kv6_render(
		(struct kv6_t*[]) {
			&model_semi_tracer,
			&model_smg_tracer,
			&model_shotgun_tracer,
		}[t->type],
		TEAM_SPECTATOR);
	matrix_pop();

	return false;
}

void tracer_render() {
	entitysys_iterate(&tracers, NULL, tracer_render_single);
}

static bool tracer_update_single(void* obj, void* user) {
	struct Tracer* t = (struct Tracer*)obj;
	float dt = *(float*)user;

	float len = distance3D(t->x, t->y, t->z, t->r.origin.x, t->r.origin.y, t->r.origin.z);

	// 128.0[m] / 256.0[m/s] = 0.5[s]
	if((t->hit.type != CAMERA_HITTYPE_NONE && len > pow(t->hit.distance, 2)) || window_time() - t->created > 0.5F) {
		if(t->hit.type != CAMERA_HITTYPE_NONE)
			sound_create(SOUND_WORLD, &sound_impact, t->r.origin.x, t->r.origin.y, t->r.origin.z);

		return true;
	} else {
		t->r.origin.x += t->r.direction.x * 32.0F * dt;
		t->r.origin.y += t->r.direction.y * 32.0F * dt;
		t->r.origin.z += t->r.direction.z * 32.0F * dt;
	}

	return false;
}

void tracer_update(float dt) {
	entitysys_iterate(&tracers, &dt, tracer_update_single);
}

void tracer_init() {
	entitysys_create(&tracers, sizeof(struct Tracer), PLAYERS_MAX);
}
