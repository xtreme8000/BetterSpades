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

struct Particle* particles;
static int particle_insert = 0;
static int particle_remove = 0;
static struct tesselator particle_tesselator;

void particle_init() {
	particles = calloc(PARTICLES_MAX, sizeof(struct Particle));
	CHECK_ALLOCATION_ERROR(particles)

	tesselator_create(&particle_tesselator, VERTEX_FLOAT, 0);
}

static void particle_update_single(struct Particle* p, float dt) {
	float size = p->size * (1.0F - ((float)(window_time() - p->fade) / 2.0F));

	if(size < 0.01F) {
		particle_remove = (particle_remove + 1) % PARTICLES_MAX; // allowed, particles are sorted by creation time
	} else {
		float acc_y = -32.0F * dt;
		if(map_isair(p->x, p->y + acc_y * dt - p->size / 2.0F, p->z) && !p->y + acc_y * dt < 0.0F) {
			p->vy += acc_y;
		}
		float movement_x = p->vx * dt;
		float movement_y = p->vy * dt;
		float movement_z = p->vz * dt;
		int on_ground = 0;
		if(!map_isair(p->x + movement_x, p->y, p->z)) {
			movement_x = 0.0F;
			p->vx = -p->vx * 0.6F;
			on_ground = 1;
		}
		if(!map_isair(p->x + movement_x, p->y + movement_y, p->z)) {
			movement_y = 0.0F;
			p->vy = -p->vy * 0.6F;
			on_ground = 1;
		}
		if(!map_isair(p->x + movement_x, p->y + movement_y, p->z + movement_z)) {
			movement_z = 0.0F;
			p->vz = -p->vz * 0.6F;
			on_ground = 1;
		}
		// air and ground friction
		if(on_ground) {
			p->vx *= pow(0.1F, dt);
			p->vy *= pow(0.1F, dt);
			p->vz *= pow(0.1F, dt);
			if(abs(p->vx) < 0.1F)
				p->vx = 0.0F;
			if(abs(p->vy) < 0.1F)
				p->vy = 0.0F;
			if(abs(p->vz) < 0.1F)
				p->vz = 0.0F;
		} else {
			p->vx *= pow(0.4F, dt);
			p->vy *= pow(0.4F, dt);
			p->vz *= pow(0.4F, dt);
		}
		p->x += movement_x;
		p->y += movement_y;
		p->z += movement_z;
	}
}

void particle_update(float dt) {
	if(particle_remove <= particle_insert) {
		for(int k = particle_remove; k < particle_insert; k++)
			particle_update_single(particles + k, dt);
	} else {
		for(int k = particle_remove; k < PARTICLES_MAX; k++)
			particle_update_single(particles + k, dt);
		for(int k = 0; k < particle_insert; k++)
			particle_update_single(particles + k, dt);
	}
}

static void particle_render_single(struct tesselator* tess, struct Particle* p) {
	if(distance2D(camera_x, camera_z, p->x, p->z) > pow(settings.render_distance, 2.0F))
		return;

	float size = p->size / 2.0F * (1.0F - ((float)(window_time() - p->fade) / 2.0F));

	if(p->type == 255) {
		tesselator_set_color(tess, p->color);

		tesselator_addf_simple(tess,
							   (float[]) {p->x - size, p->y - size, p->z - size, p->x + size, p->y - size, p->z - size,
										  p->x + size, p->y - size, p->z + size, p->x - size, p->y - size,
										  p->z + size});

		tesselator_addf_simple(tess,
							   (float[]) {p->x - size, p->y + size, p->z - size, p->x - size, p->y + size, p->z + size,
										  p->x + size, p->y + size, p->z + size, p->x + size, p->y + size,
										  p->z - size});

		tesselator_addf_simple(tess,
							   (float[]) {p->x - size, p->y - size, p->z - size, p->x - size, p->y - size, p->z + size,
										  p->x - size, p->y + size, p->z + size, p->x - size, p->y + size,
										  p->z - size});

		tesselator_addf_simple(tess,
							   (float[]) {p->x + size, p->y - size, p->z - size, p->x + size, p->y + size, p->z - size,
										  p->x + size, p->y + size, p->z + size, p->x + size, p->y - size,
										  p->z + size});

		tesselator_addf_simple(tess,
							   (float[]) {p->x - size, p->y - size, p->z - size, p->x - size, p->y + size, p->z - size,
										  p->x + size, p->y + size, p->z - size, p->x + size, p->y - size,
										  p->z - size});

		tesselator_addf_simple(tess,
							   (float[]) {p->x - size, p->y - size, p->z + size, p->x + size, p->y - size, p->z + size,
										  p->x + size, p->y + size, p->z + size, p->x - size, p->y + size,
										  p->z + size});
	} else {
		struct kv6_t* casing = weapon_casing(p->type);

		if(casing) {
			matrix_push();
			matrix_identity();
			matrix_translate(p->x, p->y, p->z);
			matrix_pointAt(p->ox, p->oy * max(1.0F - (window_time() - p->fade) / 0.5F, 0.0F), p->oz);
			matrix_rotate(90.0F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			kv6_render(casing, TEAM_SPECTATOR);
			matrix_pop();
		}
	}
}

void particle_render() {
	tesselator_clear(&particle_tesselator);

	if(particle_remove <= particle_insert) {
		for(int k = particle_remove; k < particle_insert; k++)
			particle_render_single(&particle_tesselator, particles + k);
	} else {
		for(int k = particle_remove; k < PARTICLES_MAX; k++)
			particle_render_single(&particle_tesselator, particles + k);
		for(int k = 0; k < particle_insert; k++)
			particle_render_single(&particle_tesselator, particles + k);
	}

	matrix_upload();
	tesselator_draw(&particle_tesselator, 1);
}

void particle_create_casing(struct Player* p) {
	struct Particle* pp = particles + particle_insert;
	particle_insert = (particle_insert + 1) % PARTICLES_MAX;

	pp->size = 0.1F;
	pp->x = p->gun_pos.x;
	pp->y = p->gun_pos.y;
	pp->z = p->gun_pos.z;
	pp->ox = p->orientation.x;
	pp->oy = p->orientation.y;
	pp->oz = p->orientation.z;
	pp->vx = p->casing_dir.x * 3.5F;
	pp->vy = p->casing_dir.y * 3.5F;
	pp->vz = p->casing_dir.z * 3.5F;
	pp->fade = window_time();
	pp->type = p->weapon;
	pp->color = 0x00FFFF;
}

void particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount,
					 float min_size, float max_size) {
	for(int k = 0; k < amount; k++) {
		struct Particle* pp = particles + particle_insert;
		particle_insert = (particle_insert + 1) % PARTICLES_MAX;
		if(particle_insert == particle_remove)
			particle_remove = (particle_remove + 1) % PARTICLES_MAX;

		pp->size = ((float)rand() / (float)RAND_MAX) * (max_size - min_size) + min_size;
		pp->x = x;
		pp->y = y;
		pp->z = z;
		pp->vx = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		pp->vy = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		pp->vz = (((float)rand() / (float)RAND_MAX) * 2.0F - 1.0F);
		float len = sqrt(pp->vx * pp->vx + pp->vy * pp->vy + pp->vz * pp->vz);
		pp->vx = (pp->vx / len) * velocity;
		pp->vy = (pp->vy / len) * velocity * velocity_y;
		pp->vz = (pp->vz / len) * velocity;
		pp->fade = window_time();
		pp->color = color;
		pp->type = 255;
	}
}
