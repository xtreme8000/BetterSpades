/*
	Copyright (c) 2017-2018 ByteBit

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

struct Particle {
	float x,y,z;
	float vx,vy,vz;
	float ox,oy,oz;
	unsigned char type;
	float size;
	float fade;
	unsigned int color;
};

extern struct Particle* particles;
extern float* particles_vertices;
extern unsigned char* particles_colors;
#define PARTICLES_MAX 8192

void particle_init(void);
void particle_update(float dt);
int particle_render(void);
void particle_create_casing(struct Player* p);
void particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount, float min_size, float max_size);
