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
#include <math.h>

#include "common.h"
#include "window.h"
#include "camera.h"
#include "map.h"
#include "matrix.h"
#include "particle.h"
#include "model.h"
#include "weapon.h"
#include "config.h"
#include "tesselator.h"
#include "entitysystem.h"

struct entity_system particles;
struct tesselator particle_tesselator;

void particle_init() {
	entitysys_create(&particles, sizeof(struct Particle), 256);
	tesselator_create(&particle_tesselator, VERTEX_FLOAT, 0);
}

static bool particle_update_single(void* obj, void* user) {
	struct Particle* p = (struct Particle*)obj;
	float dt = *(float*)user;
	float size = p->size * (1.0F - ((float)(window_time() - p->fade) / 2.0F));

	if(size < 0.01F) {
		return true;
	} else {
		float acc_y = -32.0F * dt;

		if(map_isair(p->x, p->y + acc_y * dt - p->size / 2.0F, p->z) && !p->y + acc_y * dt < 0.0F) {
			p->vy += acc_y;
		}

		float movement_x = p->vx * dt;
		float movement_y = p->vy * dt;
		float movement_z = p->vz * dt;
		bool on_ground = false;

		if(!map_isair(p->x + movement_x, p->y, p->z)) {
			movement_x = 0.0F;
			p->vx = -p->vx * 0.6F;
			on_ground = true;
		}
		if(!map_isair(p->x + movement_x, p->y + movement_y, p->z)) {
			movement_y = 0.0F;
			p->vy = -p->vy * 0.6F;
			on_ground = true;
		}
		if(!map_isair(p->x + movement_x, p->y + movement_y, p->z + movement_z)) {
			movement_z = 0.0F;
			p->vz = -p->vz * 0.6F;
			on_ground = true;
		}

		float pow1_tys = 0.999991F + (2.55114F * dt - 2.30093F) * dt;
		float pow4_tys = 1.0F + (0.413432 * dt - 0.916185F) * dt;

		// air and ground friction
		if(on_ground) {
			p->vx *= pow1_tys; // pow(0.1F, dt);
			p->vy *= pow1_tys; // pow(0.1F, dt);
			p->vz *= pow1_tys; // pow(0.1F, dt);

			if(abs(p->vx) < 0.1F)
				p->vx = 0.0F;
			if(abs(p->vy) < 0.1F)
				p->vy = 0.0F;
			if(abs(p->vz) < 0.1F)
				p->vz = 0.0F;
		} else {
			p->vx *= pow4_tys; // pow(0.4F, dt);
			p->vy *= pow4_tys; // pow(0.4F, dt);
			p->vz *= pow4_tys; // pow(0.4F, dt);
		}

		p->x += movement_x;
		p->y += movement_y;
		p->z += movement_z;

		return false;
	}
}

void particle_update(float dt) {
	entitysys_iterate(&particles, &dt, particle_update_single);
}

static bool particle_render_single(void* obj, void* user) {
	struct Particle* p = (struct Particle*)obj;
	struct tesselator* tess = (struct tesselator*)user;

	if(distance2D(camera_x, camera_z, p->x, p->z) > settings.render_distance * settings.render_distance)
		return false;

	float size = p->size / 2.0F * (1.0F - ((float)(window_time() - p->fade) / 2.0F));

	if(p->type == 255) {
		tesselator_set_color(tess, p->color);

		tesselator_addf_cube_face(tess, CUBE_FACE_X_N, p->x - size, p->y - size, p->z - size, size * 2.0F);
		tesselator_addf_cube_face(tess, CUBE_FACE_X_P, p->x - size, p->y - size, p->z - size, size * 2.0F);
		tesselator_addf_cube_face(tess, CUBE_FACE_Y_N, p->x - size, p->y - size, p->z - size, size * 2.0F);
		tesselator_addf_cube_face(tess, CUBE_FACE_Y_P, p->x - size, p->y - size, p->z - size, size * 2.0F);
		tesselator_addf_cube_face(tess, CUBE_FACE_Z_N, p->x - size, p->y - size, p->z - size, size * 2.0F);
		tesselator_addf_cube_face(tess, CUBE_FACE_Z_P, p->x - size, p->y - size, p->z - size, size * 2.0F);
	} else {
		struct kv6_t* casing = weapon_casing(p->type);

		if(casing) {
			matrix_push(matrix_model);
			matrix_identity(matrix_model);
			matrix_translate(matrix_model, p->x, p->y, p->z);
			matrix_pointAt(matrix_model, p->ox, p->oy * max(1.0F - (window_time() - p->fade) / 0.5F, 0.0F), p->oz);
			matrix_rotate(matrix_model, 90.0F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			kv6_render(casing, TEAM_SPECTATOR);
			matrix_pop(matrix_model);
		}
	}

	return false;
}

void particle_render() {
	tesselator_clear(&particle_tesselator);

	entitysys_iterate(&particles, &particle_tesselator, particle_render_single);

	matrix_upload();
	tesselator_draw(&particle_tesselator, 1);
}

void particle_create_casing(struct Player* p) {
	entitysys_add(&particles,
				  &(struct Particle) {
					  .size = 0.1F,
					  .x = p->gun_pos.x,
					  .y = p->gun_pos.y,
					  .z = p->gun_pos.z,
					  .ox = p->orientation.x,
					  .oy = p->orientation.y,
					  .oz = p->orientation.z,
					  .vx = p->casing_dir.x * 3.5F,
					  .vy = p->casing_dir.y * 3.5F,
					  .vz = p->casing_dir.z * 3.5F,
					  .fade = window_time(),
					  .type = p->weapon,
					  .color = 0x00FFFF,
				  });
}

void particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount,
					 float min_size, float max_size) {
	for(int k = 0; k < amount; k++) {
		float vx = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		float vy = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		float vz = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		float len = len3D(vx, vy, vz);

		vx = (vx / len) * velocity;
		vy = (vy / len) * velocity * velocity_y;
		vz = (vz / len) * velocity;

		entitysys_add(&particles,
					  &(struct Particle) {
						  .size = ((float)rand() / (float)RAND_MAX) * (max_size - min_size) + min_size,
						  .x = x,
						  .y = y,
						  .z = z,
						  .vx = vx,
						  .vy = vy,
						  .vz = vz,
						  .fade = window_time(),
						  .color = color,
						  .type = 255,
					  });
	}
}
