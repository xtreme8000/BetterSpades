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

#include <math.h>

#include "window.h"
#include "particle.h"
#include "matrix.h"
#include "model.h"
#include "sound.h"
#include "grenade.h"
#include "map.h"
#include "entitysystem.h"

struct entity_system grenades;

void grenade_init() {
	entitysys_create(&grenades, sizeof(struct Grenade), 32);
}

void grenade_add(struct Grenade* g) {
	g->created = window_time();

	entitysys_add(&grenades, g);
}

static int grenade_clipworld(int x, int y, int z) {
	if(x < 0)
		x += map_size_x;
	if(y < 0)
		y += map_size_z;
	if(x >= map_size_x)
		x -= map_size_x;
	if(y >= map_size_z)
		y -= map_size_z;

	int sz;

	if(z < 0)
		return 0;
	sz = (int)z;
	if(sz == map_size_y - 1)
		sz = map_size_y - 2;
	else if(sz >= map_size_y - 1)
		return 1;
	else if(sz < 0)
		return 0;
	return !map_isair((int)x, (map_size_y - 1) - sz, (int)y);
}

static int grenade_move(struct Grenade* g, float dt) {
	float tmp;
	tmp = g->pos.z;
	g->pos.z = 63.0F - g->pos.y;
	g->pos.y = tmp;
	tmp = g->velocity.z;
	g->velocity.z = -g->velocity.y;
	g->velocity.y = tmp;

	struct Position fpos = g->pos; // old position

	// do velocity & gravity (friction is negligible)
	float f = dt * 32.0F;
	g->velocity.z += dt;
	g->pos.x += g->velocity.x * f;
	g->pos.y += g->velocity.y * f;
	g->pos.z += g->velocity.z * f;
	// do rotation
	// FIX ME: Loses orientation after 45 degree bounce off wall
	// if(g->v.x > 0.1f || g->v.x < -0.1f || g->v.y > 0.1f || g->v.y < -0.1f)
	// {
	// f *= -0.5;
	// }
	// make it bounce (accurate)
	int ret = 0;

	int lpx = floor(g->pos.x);
	int lpy = floor(g->pos.y);
	int lpz = floor(g->pos.z);

	if(grenade_clipworld(lpx, lpy, lpz)) { // hit a wall

		ret = 1;
		if(fabs(g->velocity.x) > 0.1F || fabs(g->velocity.y) > 0.1F || fabs(g->velocity.z) > 0.1F)
			ret = 2; // play sound

		int lp2x = floor(fpos.x);
		int lp2y = floor(fpos.y);
		int lp2z = floor(fpos.z);
		if(lpz != lp2z && ((lpx == lp2x && lpy == lp2y) || !grenade_clipworld(lpx, lpy, lp2z)))
			g->velocity.z *= -1;
		else if(lpx != lp2x && ((lpy == lp2y && lpz == lp2z) || !grenade_clipworld(lp2x, lpy, lpz)))
			g->velocity.x *= -1;
		else if(lpy != lp2y && ((lpx == lp2x && lpz == lp2z) || !grenade_clipworld(lpx, lp2y, lpz)))
			g->velocity.y *= -1;
		g->pos = fpos; // set back to old position
		g->velocity.x *= 0.36F;
		g->velocity.y *= 0.36F;
		g->velocity.z *= 0.36F;
	}

	tmp = g->pos.y;
	g->pos.y = 63.0F - g->pos.z;
	g->pos.z = tmp;
	tmp = g->velocity.y;
	g->velocity.y = -g->velocity.z;
	g->velocity.z = tmp;

	return ret;
}

static int grenade_inwater(struct Grenade* g) {
	return g->pos.y < 1.0F;
}

bool grenade_render_single(void* obj, void* user) {
	struct Grenade* g = (struct Grenade*)obj;

	// TODO: position grenade on ground properly
	matrix_push();
	matrix_translate(g->pos.x, g->pos.y + (model_grenade.zpiv + model_grenade.zsiz * 2) * model_grenade.scale,
					 g->pos.z);
	if(fabs(g->velocity.x) > 0.05F || fabs(g->velocity.y) > 0.05F || fabs(g->velocity.z) > 0.05F)
		matrix_rotate(-window_time() * 720.0F, -g->velocity.z, 0.0F, g->velocity.x);
	matrix_upload();

	kv6_calclight(g->pos.x, g->pos.y, g->pos.z);

	kv6_render(&model_grenade, TEAM_1);
	matrix_pop();
	return false;
}

void grenade_render() {
	entitysys_iterate(&grenades, NULL, grenade_render_single);
}

bool grenade_update_single(void* obj, void* user) {
	struct Grenade* g = (struct Grenade*)obj;
	float dt = *(float*)user;

	if(window_time() - g->created > g->fuse_length) {
		sound_create(SOUND_WORLD, grenade_inwater(g) ? &sound_explode_water : &sound_explode, g->pos.x, g->pos.y,
					 g->pos.z);
		particle_create(grenade_inwater(g) ? map_get(g->pos.x, 0, g->pos.z) : 0x505050, g->pos.x, g->pos.y + 1.5F,
						g->pos.z, 20.0F, 1.5F, 64, 0.1F, 0.5F);

		return true;
	} else {
		if(grenade_move(g, dt) == 2)
			sound_create(SOUND_WORLD, &sound_grenade_bounce, g->pos.x, g->pos.y, g->pos.z);

		return false;
	}
}

void grenade_update(float dt) {
	entitysys_iterate(&grenades, &dt, grenade_update_single);
}
