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

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>
#include <stdbool.h>

#include "aabb.h"
#include "glx.h"
#include "tesselator.h"

#define KV6_VIS_NEG_X (1 << 0)
#define KV6_VIS_POS_X (1 << 1)
#define KV6_VIS_NEG_Z (1 << 2)
#define KV6_VIS_POS_Z (1 << 3)
#define KV6_VIS_POS_Y (1 << 4)
#define KV6_VIS_NEG_Y (1 << 5)

struct kv6_voxel {
	uint16_t x, y, z;
	uint8_t visfaces;
	uint32_t color;
};

struct kv6_t {
	uint16_t xsiz, ysiz, zsiz;
	float xpiv, ypiv, zpiv;
	bool has_display_list, colorize;
	struct glx_displaylist display_list[2];
	struct kv6_voxel* voxels;
	int voxel_count;
	float scale;
	float red, green, blue;
};

extern struct kv6_t model_playerdead;
extern struct kv6_t model_playerhead;
extern struct kv6_t model_playertorso;
extern struct kv6_t model_playertorsoc;
extern struct kv6_t model_playerarms;
extern struct kv6_t model_playerleg;
extern struct kv6_t model_playerlegc;
extern struct kv6_t model_intel;
extern struct kv6_t model_tent;

extern struct kv6_t model_semi;
extern struct kv6_t model_smg;
extern struct kv6_t model_shotgun;
extern struct kv6_t model_spade;
extern struct kv6_t model_block;
extern struct kv6_t model_grenade;

extern struct kv6_t model_semi_tracer;
extern struct kv6_t model_smg_tracer;
extern struct kv6_t model_shotgun_tracer;

extern struct kv6_t model_semi_casing;
extern struct kv6_t model_smg_casing;
extern struct kv6_t model_shotgun_casing;

void kv6_calclight(int x, int y, int z);
void kv6_rebuild_complete(void);
void kv6_rebuild(struct kv6_t* kv6);
void kv6_render(struct kv6_t* kv6, unsigned char team);
void kv6_load(struct kv6_t* kv6, void* bytes, float scale);
void kv6_init(void);

extern float kv6_normals[256][3];

#endif
