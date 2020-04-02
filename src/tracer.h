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

#ifndef TRACER_H
#define TRACER_H

#include "aabb.h"
#include "player.h"
#include "camera.h"

#define TRACER_MAX (PLAYERS_MAX * 5)

struct Tracer {
	struct Camera_HitType hit;
	float x, y, z;
	Ray r;
	int type;
	float created;
	int used;
};

void tracer_minimap(int large, float scalef, float minimap_x, float minimap_y);
void tracer_pvelocity(float* o, struct Player* p);
void tracer_add(int type, float x, float y, float z, float dx, float dy, float dz);
void tracer_update(float dt);
void tracer_render(void);
void tracer_init(void);

#endif
