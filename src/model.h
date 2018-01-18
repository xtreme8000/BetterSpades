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

struct kv6_voxel {
	short x,y,z;
	unsigned char visfaces;
	unsigned int color;
};

struct kv6_t {
	unsigned short xsiz, ysiz, zsiz;
	float xpiv, ypiv, zpiv;
	unsigned char has_display_list, colorize;
	int display_list;
	struct kv6_voxel* voxels;
	int voxel_count;
	float scale;
	unsigned char* colors_final;
	float* vertices_final;
	char* normals_final;
	int size;
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

char kv6_intersection(struct kv6_t* kv6, Ray* r);
void kv6_rebuild_all(void);
void kv6_rebuild(struct kv6_t* kv6);
void kv6_render(struct kv6_t* kv6, unsigned char team);
void kv6_load(struct kv6_t* kv6, unsigned char* bytes, float scale);
void kv6_init(void);
void mul_matrix_vector(float* out, double* m, float* v);

extern float kv6_normals[256][3];
