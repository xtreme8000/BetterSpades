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

#include "common.h"

struct Particle* particles;
float* particles_vertices;
unsigned char* particles_colors;
static int particle_insert = 0;
static int particle_remove = 0;

void particle_init() {
	particles = malloc(sizeof(struct Particle) * PARTICLES_MAX);
	CHECK_ALLOCATION_ERROR(particles)
	memset(particles, 0, sizeof(struct Particle) * PARTICLES_MAX);

	particles_vertices = malloc(sizeof(float) * 72 * PARTICLES_MAX);
	CHECK_ALLOCATION_ERROR(particles_vertices)
	particles_colors = malloc(sizeof(unsigned char) * 72 * PARTICLES_MAX);
	CHECK_ALLOCATION_ERROR(particles_colors)
}

static void particle_update_single(int k, float dt) {
	float size = particles[k].size * (1.0F - ((float)(window_time() - particles[k].fade) / 2.0F));

	if(size < 0.01F) {
		particle_remove = (particle_remove + 1) % PARTICLES_MAX; // allowed, particles are sorted by creation time
	} else {
		float acc_y = -32.0F * dt;
		if(map_isair(particles[k].x, particles[k].y + acc_y * dt - particles[k].size / 2.0F, particles[k].z)
		   && !particles[k].y + acc_y * dt < 0.0F) {
			particles[k].vy += acc_y;
		}
		float movement_x = particles[k].vx * dt;
		float movement_y = particles[k].vy * dt;
		float movement_z = particles[k].vz * dt;
		int on_ground = 0;
		if(!map_isair(particles[k].x + movement_x, particles[k].y, particles[k].z)) {
			movement_x = 0.0F;
			particles[k].vx = -particles[k].vx * 0.6F;
			on_ground = 1;
		}
		if(!map_isair(particles[k].x + movement_x, particles[k].y + movement_y, particles[k].z)) {
			movement_y = 0.0F;
			particles[k].vy = -particles[k].vy * 0.6F;
			on_ground = 1;
		}
		if(!map_isair(particles[k].x + movement_x, particles[k].y + movement_y, particles[k].z + movement_z)) {
			movement_z = 0.0F;
			particles[k].vz = -particles[k].vz * 0.6F;
			on_ground = 1;
		}
		// air and ground friction
		if(on_ground) {
			particles[k].vx *= pow(0.1F, dt);
			particles[k].vy *= pow(0.1F, dt);
			particles[k].vz *= pow(0.1F, dt);
			if(abs(particles[k].vx) < 0.1F)
				particles[k].vx = 0.0F;
			if(abs(particles[k].vy) < 0.1F)
				particles[k].vy = 0.0F;
			if(abs(particles[k].vz) < 0.1F)
				particles[k].vz = 0.0F;
		} else {
			particles[k].vx *= pow(0.4F, dt);
			particles[k].vy *= pow(0.4F, dt);
			particles[k].vz *= pow(0.4F, dt);
		}
		particles[k].x += movement_x;
		particles[k].y += movement_y;
		particles[k].z += movement_z;
	}
}

void particle_update(float dt) {
	if(particle_remove <= particle_insert) {
		for(int k = particle_remove; k < particle_insert; k++)
			particle_update_single(k, dt);
	} else {
		for(int k = particle_remove; k < PARTICLES_MAX; k++)
			particle_update_single(k, dt);
		for(int k = 0; k < particle_insert; k++)
			particle_update_single(k, dt);
	}
}

static void particle_render_single(int k, int* vertex_index, int* color_index) {
	if(distance2D(camera_x, camera_z, particles[k].x, particles[k].z) > pow(settings.render_distance, 2.0F))
		return;

	float size = particles[k].size / 2.0F * (1.0F - ((float)(window_time() - particles[k].fade) / 2.0F));

	if(particles[k].type == 255) {
		for(int i = 0; i < 24; i++) {
			particles_colors[(*color_index)++] = particles[k].color & 0xFF;
			particles_colors[(*color_index)++] = (particles[k].color >> 8) & 0xFF;
			particles_colors[(*color_index)++] = (particles[k].color >> 16) & 0xFF;
		}

		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;

		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;

		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;

		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;

		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z - size;

		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y - size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x + size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
		particles_vertices[(*vertex_index)++] = particles[k].x - size;
		particles_vertices[(*vertex_index)++] = particles[k].y + size;
		particles_vertices[(*vertex_index)++] = particles[k].z + size;
	} else {
		struct kv6_t* casing = weapon_casing(particles[k].type);
		if(casing) {
			matrix_push();
			matrix_identity();
			matrix_translate(particles[k].x, particles[k].y, particles[k].z);
			matrix_pointAt(particles[k].ox,
						   particles[k].oy * max(1.0F - (window_time() - particles[k].fade) / 0.5F, 0.0F),
						   particles[k].oz);
			matrix_rotate(90.0F, 0.0F, 1.0F, 0.0F);
			matrix_upload();
			kv6_render(casing, TEAM_SPECTATOR);
			matrix_pop();
		}
	}
}

int particle_render() {
	int color_index = 0;
	int vertex_index = 0;

#ifndef OPENGL_ES
	if(particle_remove <= particle_insert) {
		for(int k = particle_remove; k < particle_insert; k++)
			particle_render_single(k, &vertex_index, &color_index);
	} else {
		for(int k = particle_remove; k < PARTICLES_MAX; k++)
			particle_render_single(k, &vertex_index, &color_index);
		for(int k = 0; k < particle_insert; k++)
			particle_render_single(k, &vertex_index, &color_index);
	}

	if(vertex_index > 0) {
		matrix_upload();
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, particles_vertices);
		glColorPointer(3, GL_UNSIGNED_BYTE, 0, particles_colors);
		glDrawArrays(GL_QUADS, 0, vertex_index / 3);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
#endif

	return vertex_index / 72;
}

void particle_create_casing(struct Player* p) {
	struct Particle* pp = &particles[particle_insert];
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
		struct Particle* pp = &particles[particle_insert];
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
