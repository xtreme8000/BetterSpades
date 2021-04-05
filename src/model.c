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
#include "player.h"
#include "file.h"
#include "camera.h"
#include "matrix.h"
#include "log.h"
#include "map.h"
#include "config.h"
#include "model.h"
#include "model_normals.h"
#include "texture.h"

struct kv6_t model_playerdead;
struct kv6_t model_playerhead;
struct kv6_t model_playertorso;
struct kv6_t model_playertorsoc;
struct kv6_t model_playerarms;
struct kv6_t model_playerleg;
struct kv6_t model_playerlegc;
struct kv6_t model_intel;
struct kv6_t model_tent;

struct kv6_t model_semi;
struct kv6_t model_smg;
struct kv6_t model_shotgun;
struct kv6_t model_spade;
struct kv6_t model_block;
struct kv6_t model_grenade;

struct kv6_t model_semi_tracer;
struct kv6_t model_smg_tracer;
struct kv6_t model_shotgun_tracer;

struct kv6_t model_semi_casing;
struct kv6_t model_smg_casing;
struct kv6_t model_shotgun_casing;

static void kv6_load_file(struct kv6_t* kv6, char* filename, float scale) {
	void* data = file_load(filename);
	kv6_load(kv6, data, scale);
	free(data);
}

static void kv6_check_dimensions(struct kv6_t* kv6, float max) {
	if(max(max(kv6->xsiz, kv6->ysiz), kv6->zsiz) * kv6->scale > max) {
		log_error("Model dimensions too large");
		kv6->voxel_count = 0;
	}
}

void kv6_init() {
	kv6_load_file(&model_playerdead, "kv6/playerdead.kv6", 0.1F);
	kv6_load_file(&model_playerhead, "kv6/playerhead.kv6", 0.1F);
	kv6_load_file(&model_playertorso, "kv6/playertorso.kv6", 0.1F);
	kv6_load_file(&model_playertorsoc, "kv6/playertorsoc.kv6", 0.1F);
	kv6_load_file(&model_playerarms, "kv6/playerarms.kv6", 0.1F);
	kv6_load_file(&model_playerleg, "kv6/playerleg.kv6", 0.1F);
	kv6_load_file(&model_playerlegc, "kv6/playerlegc.kv6", 0.1F);

	kv6_load_file(&model_intel, "kv6/intel.kv6", 0.2F);
	kv6_load_file(&model_tent, "kv6/cp.kv6", 0.278F);

	kv6_load_file(&model_semi, "kv6/semi.kv6", 0.05F);
	kv6_load_file(&model_smg, "kv6/smg.kv6", 0.05F);
	kv6_load_file(&model_shotgun, "kv6/shotgun.kv6", 0.05F);
	kv6_load_file(&model_spade, "kv6/spade.kv6", 0.05F);
	kv6_load_file(&model_block, "kv6/block.kv6", 0.05F);
	model_block.colorize = true;
	kv6_load_file(&model_grenade, "kv6/grenade.kv6", 0.05F);

	kv6_load_file(&model_semi_tracer, "kv6/semitracer.kv6", 0.05F);
	kv6_load_file(&model_smg_tracer, "kv6/smgtracer.kv6", 0.05F);
	kv6_load_file(&model_shotgun_tracer, "kv6/shotguntracer.kv6", 0.05F);

	kv6_load_file(&model_semi_casing, "kv6/semicasing.kv6", 0.0125F);
	kv6_load_file(&model_smg_casing, "kv6/smgcasing.kv6", 0.0125F);
	kv6_load_file(&model_shotgun_casing, "kv6/shotguncasing.kv6", 0.0125F);

	kv6_check_dimensions(&model_playerhead, 1.2F);
	kv6_check_dimensions(&model_playertorso, 1.8F);
	kv6_check_dimensions(&model_playertorsoc, 1.6F);
	kv6_check_dimensions(&model_playerarms, 2.0F);
	kv6_check_dimensions(&model_playerleg, 2.0F);
	kv6_check_dimensions(&model_playerlegc, 1.6F);

	kv6_check_dimensions(&model_semi, 2.25F);
	kv6_check_dimensions(&model_smg, 2.25F);
	kv6_check_dimensions(&model_shotgun, 2.25F);
	kv6_check_dimensions(&model_spade, 2.25F);
	kv6_check_dimensions(&model_block, 2.25F);
	kv6_check_dimensions(&model_grenade, 2.25F);
}

void kv6_rebuild_complete() {
	kv6_rebuild(&model_playerdead);
	kv6_rebuild(&model_playerhead);
	kv6_rebuild(&model_playertorso);
	kv6_rebuild(&model_playertorsoc);
	kv6_rebuild(&model_playerarms);
	kv6_rebuild(&model_playerleg);
	kv6_rebuild(&model_playerlegc);
	kv6_rebuild(&model_intel);
	kv6_rebuild(&model_tent);
	kv6_rebuild(&model_semi);
	kv6_rebuild(&model_smg);
	kv6_rebuild(&model_shotgun);
	kv6_rebuild(&model_spade);
	kv6_rebuild(&model_block);
	kv6_rebuild(&model_grenade);
	kv6_rebuild(&model_semi_tracer);
	kv6_rebuild(&model_smg_tracer);
	kv6_rebuild(&model_shotgun_tracer);
	kv6_rebuild(&model_semi_casing);
	kv6_rebuild(&model_smg_casing);
	kv6_rebuild(&model_shotgun_casing);
}

void kv6_load(struct kv6_t* kv6, void* bytes, float scale) {
	kv6->colorize = false;
	kv6->has_display_list = false;
	kv6->scale = scale;

	size_t index = 0;
	if(buffer_read32(bytes, index) == 0x6C78764B) { //"Kvxl"
		index += 4;
		kv6->xsiz = buffer_read32(bytes, index);
		index += 4;
		kv6->ysiz = buffer_read32(bytes, index);
		index += 4;
		kv6->zsiz = buffer_read32(bytes, index);
		index += 4;

		kv6->xpiv = buffer_readf(bytes, index);
		index += 4;
		kv6->ypiv = buffer_readf(bytes, index);
		index += 4;
		kv6->zpiv = kv6->zsiz - buffer_readf(bytes, index);
		index += 4;

		kv6->voxel_count = buffer_read32(bytes, index);
		index += 4;

		kv6->voxels = malloc(sizeof(struct kv6_voxel) * kv6->voxel_count);
		CHECK_ALLOCATION_ERROR(kv6->voxels)

		for(size_t k = 0; k < kv6->voxel_count; k++) {
			uint32_t color = buffer_read32(bytes, index);
			index += 4;
			uint16_t zpos = buffer_read16(bytes, index);
			index += 2;
			uint8_t visfaces = buffer_read8(bytes, index++); // 0x00zZyYxX
			uint8_t lighting = buffer_read8(bytes, index++); // compressed normal vector (also referred to as lighting)

			kv6->voxels[k] = (struct kv6_voxel) {
				.color = (color & 0xFFFFFF) | (lighting << 24),
				.visfaces = visfaces,
				.z = (kv6->zsiz - 1) - zpos,
			};
		}

		index += 4 * kv6->xsiz;

		struct kv6_voxel* voxel = kv6->voxels;

		for(size_t x = 0; x < kv6->xsiz; x++) {
			for(size_t y = 0; y < kv6->ysiz; y++) {
				uint16_t size = buffer_read16(bytes, index);
				index += 2;

				for(size_t z = 0; z < size; z++, voxel++) {
					voxel->x = x;
					voxel->y = y;
				}
			}
		}
	} else {
		log_error("Data not in kv6 format");
		kv6->xsiz = kv6->ysiz = kv6->zsiz = 0;
		kv6->voxel_count = 0;
	}
}

void kv6_rebuild(struct kv6_t* kv6) {
	if(kv6->has_display_list) {
		glx_displaylist_destroy(kv6->display_list + 0);
		glx_displaylist_destroy(kv6->display_list + 1);
		kv6->has_display_list = false;
	}
}

void kv6_calclight(int x, int y, int z) {
	float f = 1.0F;

	if(x >= 0 && y >= 0 && z >= 0)
		f = map_sunblock(x, y, z);

	float lambient[4] = {0.5F * f, 0.5F * f, 0.5F * f, 1.0F};
	float ldiffuse[4] = {0.5F * f, 0.5F * f, 0.5F * f, 1.0F};

	glLightfv(GL_LIGHT0, GL_AMBIENT, lambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, ldiffuse);
}

static int kv6_voxel_cmp(const void* a, const void* b) {
	const struct kv6_voxel* A = a;
	const struct kv6_voxel* B = b;

	if(A->x == B->x) {
		if(A->y == B->y) {
			return B->z - A->z;
		} else {
			return A->y - B->y;
		}
	} else {
		return A->x - B->x;
	}
}

static void greedy_mesh(struct kv6_t* kv6, struct kv6_voxel* voxel, uint8_t* marked, size_t* max_a, size_t* max_b,
						uint8_t face) {
	if(!settings.greedy_meshing) {
		*max_a = 1;
		*max_b = 1;
	} else {
		switch(face) {
			case KV6_VIS_POS_X:
			case KV6_VIS_NEG_X:
				*max_a = kv6->ysiz;
				*max_b = kv6->zsiz;
				break;
			case KV6_VIS_POS_Y:
			case KV6_VIS_NEG_Y:
				*max_a = kv6->xsiz;
				*max_b = kv6->ysiz;
				break;
			case KV6_VIS_POS_Z:
			case KV6_VIS_NEG_Z:
				*max_a = kv6->xsiz;
				*max_b = kv6->zsiz;
				break;
		}

		struct kv6_voxel lookup = *voxel;

		for(size_t a = 0; a < *max_a; a++) {
			struct kv6_voxel* recent[*max_b];
			size_t b;
			for(b = 0; b < *max_b; b++) {
				switch(face) {
					case KV6_VIS_POS_X:
					case KV6_VIS_NEG_X:
						lookup.y = voxel->y + b;
						lookup.z = voxel->z - a;
						break;
					case KV6_VIS_POS_Y:
					case KV6_VIS_NEG_Y:
						lookup.x = voxel->x + a;
						lookup.y = voxel->y + b;
						break;
					case KV6_VIS_POS_Z:
					case KV6_VIS_NEG_Z:
						lookup.x = voxel->x + a;
						lookup.z = voxel->z - b;
						break;
				}

				struct kv6_voxel* neighbour
					= bsearch(&lookup, kv6->voxels, kv6->voxel_count, sizeof(struct kv6_voxel), kv6_voxel_cmp);

				if(!neighbour || !(neighbour->visfaces & face)
				   || (neighbour->color & 0xFFFFFF) != (voxel->color & 0xFFFFFF)
				   || marked[neighbour - kv6->voxels] & face) {
					break;
				} else {
					marked[neighbour - kv6->voxels] |= face;
					recent[b] = neighbour;
				}
			}

			if(a == 0)
				*max_b = b;

			if(b < *max_b) { // early abort
				*max_a = a;

				for(size_t k = 0; k < b; k++)
					marked[recent[k] - kv6->voxels] &= ~face;

				break;
			}
		}
	}
}

static int kv6_program = -1;
void kv6_render(struct kv6_t* kv6, unsigned char team) {
	if(!kv6)
		return;
	if(team == TEAM_SPECTATOR)
		team = 2;
	if(!settings.voxlap_models) {
		if(!kv6->has_display_list) {
			struct tesselator tess_color;
			tesselator_create(&tess_color, VERTEX_INT, 1);
			struct tesselator tess_team;
			tesselator_create(&tess_team, VERTEX_INT, 1);

			glx_displaylist_create(kv6->display_list + 0, true, true);
			glx_displaylist_create(kv6->display_list + 1, true, true);

			uint8_t marked[kv6->voxel_count];
			memset(marked, 0, sizeof(uint8_t) * kv6->voxel_count);

			struct kv6_voxel* voxel = kv6->voxels;
			for(size_t k = 0; k < kv6->voxel_count; k++, voxel++) {
				int b = red(voxel->color);
				int g = green(voxel->color);
				int r = blue(voxel->color);
				int a = alpha(voxel->color);

				struct tesselator* tess = &tess_color;

				if((r | g | b) == 0) {
					tess = &tess_team;
					r = g = b = 255;
				} else if(kv6->colorize) {
					r = g = b = 255;
				}

				tesselator_set_normal(tess, kv6_normals[a][0] * 128, -kv6_normals[a][2] * 128, kv6_normals[a][1] * 128);

				if(voxel->visfaces & KV6_VIS_POS_Y) {
					size_t max_x, max_z;
					greedy_mesh(kv6, voxel, marked, &max_x, &max_z, KV6_VIS_POS_Y);

					tesselator_set_color(tess, rgba(r, g, b, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_Y_P, voxel->x, voxel->z, voxel->y, max_x, 1, max_z);
				}

				if(voxel->visfaces & KV6_VIS_NEG_Y) {
					size_t max_x, max_z;
					greedy_mesh(kv6, voxel, marked, &max_x, &max_z, KV6_VIS_NEG_Y);

					tesselator_set_color(tess, rgba(r * 0.6F, g * 0.6F, b * 0.6F, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_Y_N, voxel->x, voxel->z, voxel->y, max_x, 1, max_z);
				}

				if(voxel->visfaces & KV6_VIS_NEG_Z) {
					size_t max_x, max_y;
					greedy_mesh(kv6, voxel, marked, &max_x, &max_y, KV6_VIS_NEG_Z);

					tesselator_set_color(tess, rgba(r * 0.95F, g * 0.95F, b * 0.95F, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_Z_N, voxel->x, voxel->z - (max_y - 1), voxel->y,
												  max_x, max_y, 1);
				}

				if(voxel->visfaces & KV6_VIS_POS_Z) {
					size_t max_x, max_y;
					greedy_mesh(kv6, voxel, marked, &max_x, &max_y, KV6_VIS_POS_Z);

					tesselator_set_color(tess, rgba(r * 0.9F, g * 0.9F, b * 0.9F, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_Z_P, voxel->x, voxel->z - (max_y - 1), voxel->y,
												  max_x, max_y, 1);
				}

				if(voxel->visfaces & KV6_VIS_NEG_X) {
					size_t max_y, max_z;
					greedy_mesh(kv6, voxel, marked, &max_y, &max_z, KV6_VIS_NEG_X);

					tesselator_set_color(tess, rgba(r * 0.85F, g * 0.85F, b * 0.85F, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_X_N, voxel->x, voxel->z - (max_y - 1), voxel->y, 1,
												  max_y, max_z);
				}

				if(voxel->visfaces & KV6_VIS_POS_X) {
					size_t max_y, max_z;
					greedy_mesh(kv6, voxel, marked, &max_y, &max_z, KV6_VIS_POS_X);

					tesselator_set_color(tess, rgba(r * 0.8F, g * 0.8F, b * 0.8F, 0));
					tesselator_addi_cube_face_adv(tess, CUBE_FACE_X_P, voxel->x, voxel->z - (max_y - 1), voxel->y, 1,
												  max_y, max_z);
				}
			}

			tesselator_glx(&tess_color, kv6->display_list + 0);
			tesselator_glx(&tess_team, kv6->display_list + 1);

			tesselator_free(&tess_color);
			tesselator_free(&tess_team);

			kv6->has_display_list = true;
		} else {
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glEnable(GL_COLOR_MATERIAL);
#ifndef OPENGL_ES
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
#endif
			glEnable(GL_NORMALIZE);

			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PREVIOUS);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, texture_dummy.texture_id);

			if(kv6->colorize) {
				glEnable(GL_TEXTURE_2D);
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (float[]) {kv6->red, kv6->green, kv6->blue, 1.0F});
			}

			matrix_push(matrix_model);
			matrix_scale3(matrix_model, kv6->scale);
			matrix_translate(matrix_model, -kv6->xpiv, -kv6->zpiv, -kv6->ypiv);
			matrix_upload();

			glx_displaylist_draw(kv6->display_list + 0, GLX_DISPLAYLIST_NORMAL);

			if(!kv6->colorize)
				glEnable(GL_TEXTURE_2D);

			switch(team) {
				case TEAM_1:
					glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
							   (float[]) {gamestate.team_1.red * 0.75F / 255.0F,
										  gamestate.team_1.green * 0.75F / 255.0F,
										  gamestate.team_1.blue * 0.75F / 255.0F, 1.0F});
					break;
				case TEAM_2:
					glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,
							   (float[]) {gamestate.team_2.red * 0.75F / 255.0F,
										  gamestate.team_2.green * 0.75F / 255.0F,
										  gamestate.team_2.blue * 0.75F / 255.0F, 1.0F});
					break;
				default: glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (float[]) {0, 0, 0, 1});
			}

			glx_displaylist_draw(kv6->display_list + 1, GLX_DISPLAYLIST_NORMAL);

			matrix_pop(matrix_model);

			glBindTexture(GL_TEXTURE_2D, 0);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glDisable(GL_TEXTURE_2D);

			glDisable(GL_NORMALIZE);
			glDisable(GL_COLOR_MATERIAL);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
		}
	} else {
		// render like on voxlap
		if(!kv6->has_display_list) {
			float vertices[2][kv6->voxel_count * 3];
			uint8_t colors[2][kv6->voxel_count * 4];
			int8_t normals[2][kv6->voxel_count * 3];
			kv6->has_display_list = true;

			int cnt[2] = {0, 0};

			glx_displaylist_create(kv6->display_list + 0, !kv6->colorize, true);
			glx_displaylist_create(kv6->display_list + 1, false, true);

			for(int i = 0; i < kv6->voxel_count; i++) {
				int b = red(kv6->voxels[i].color);
				int g = green(kv6->voxels[i].color);
				int r = blue(kv6->voxels[i].color);
				int a = alpha(kv6->voxels[i].color);

				int ind = ((r | g | b) == 0) ? 1 : 0;

				colors[ind][cnt[ind] * 4 + 0] = r;
				colors[ind][cnt[ind] * 4 + 1] = g;
				colors[ind][cnt[ind] * 4 + 2] = b;
				colors[ind][cnt[ind] * 4 + 3] = 255;

				normals[ind][cnt[ind] * 3 + 0] = kv6_normals[a][0] * 128;
				normals[ind][cnt[ind] * 3 + 1] = -kv6_normals[a][2] * 128;
				normals[ind][cnt[ind] * 3 + 2] = kv6_normals[a][1] * 128;

				vertices[ind][cnt[ind] * 3 + 0] = (kv6->voxels[i].x - kv6->xpiv + 0.5F) * kv6->scale;
				vertices[ind][cnt[ind] * 3 + 1] = (kv6->voxels[i].z - kv6->zpiv + 0.5F) * kv6->scale;
				vertices[ind][cnt[ind] * 3 + 2] = (kv6->voxels[i].y - kv6->ypiv + 0.5F) * kv6->scale;

				cnt[ind]++;
			}

			glx_displaylist_update(kv6->display_list + 0, cnt[0], GLX_DISPLAYLIST_POINTS, colors[0], vertices[0],
								   normals[0]);

			glx_displaylist_update(kv6->display_list + 1, cnt[1], GLX_DISPLAYLIST_POINTS, colors[1], vertices[1],
								   normals[1]);

			if(kv6_program < 0) {
				kv6_program
					= glx_shader("uniform float size;\n"
								 "uniform vec3 fog;\n"
								 "uniform vec3 camera;\n"
								 "uniform mat4 model;\n"
								 "uniform float dist_factor;\n"
								 "void main(void) {\n"
								 "	gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;\n"
								 "	float dist = length((model*gl_Vertex).xz-camera.xz)*dist_factor;\n"
								 "	vec3 N = normalize(model*vec4(gl_Normal,0)).xyz;\n"
								 "	vec3 L = normalize(vec3(0,-1,1));\n"
								 "	float d = clamp(dot(N,L),0.0,1.0)*0.5+0.5;\n"
								 "	gl_FrontColor = mix(vec4(d,d,d,1.0)*gl_Color,vec4(fog,1.0),min(dist,1.0));\n"
								 "	gl_PointSize = size/gl_Position.w;\n"
								 "}\n",
								 "void main(void) {\n"
								 "	gl_FragColor = gl_Color;\n"
								 "}\n");
			}
		}

		float near_plane_height
			= (float)settings.window_height / (2.0F * tan(glm_persp_fovy(matrix_projection) / 2.0F));

		float len_x = len3D(matrix_model[0][0], matrix_model[1][0], matrix_model[2][0]);
		float len_y = len3D(matrix_model[0][1], matrix_model[1][1], matrix_model[2][1]);
		float len_z = len3D(matrix_model[0][2], matrix_model[1][2], matrix_model[2][2]);

#ifndef OPENGL_ES
		if(!glx_version)
#endif
		{
			glPointParameterfv(GL_POINT_DISTANCE_ATTENUATION, (float[]) {0.0F, 0.0F, 1.0F});
			glPointSize(1.414F * near_plane_height * kv6->scale * (len_x + len_y + len_z) / 3.0F);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glEnable(GL_COLOR_MATERIAL);
#ifndef OPENGL_ES
			glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
#endif
			glEnable(GL_NORMALIZE);
		}

#ifndef OPENGL_ES
		if(glx_version) {
			glEnable(GL_PROGRAM_POINT_SIZE);
			glUseProgram(kv6_program);
			glUniform1f(glGetUniformLocation(kv6_program, "dist_factor"),
						glx_fog ? 1.0F / settings.render_distance : 0.0F);
			glUniform1f(glGetUniformLocation(kv6_program, "size"),
						1.414F * near_plane_height * kv6->scale * (len_x + len_y + len_z) / 3.0F);
			glUniform3f(glGetUniformLocation(kv6_program, "fog"), fog_color[0], fog_color[1], fog_color[2]);
			glUniform3f(glGetUniformLocation(kv6_program, "camera"), camera_x, camera_y, camera_z);
			glUniformMatrix4fv(glGetUniformLocation(kv6_program, "model"), 1, 0, (float*)matrix_model);
		}
#endif
		if(settings.multisamples)
			glDisable(GL_MULTISAMPLE);

		if(kv6->colorize)
			glColor3f(kv6->red, kv6->green, kv6->blue);

		glx_displaylist_draw(kv6->display_list + 0, GLX_DISPLAYLIST_POINTS);

		switch(team) {
			case TEAM_1:
				glColor3ub(gamestate.team_1.red * 0.75F, gamestate.team_1.green * 0.75F, gamestate.team_1.blue * 0.75F);
				break;
			case TEAM_2:
				glColor3ub(gamestate.team_2.red * 0.75F, gamestate.team_2.green * 0.75F, gamestate.team_2.blue * 0.75F);
				break;
			default: glColor3ub(0, 0, 0);
		}

		glx_displaylist_draw(kv6->display_list + 1, GLX_DISPLAYLIST_POINTS);

		if(settings.multisamples)
			glEnable(GL_MULTISAMPLE);
#ifndef OPENGL_ES
		if(glx_version) {
			glUseProgram(0);
			glDisable(GL_PROGRAM_POINT_SIZE);
		}
#endif

#ifndef OPENGL_ES
		if(!glx_version)
#endif
		{
			glDisable(GL_NORMALIZE);
			glDisable(GL_COLOR_MATERIAL);
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHTING);
		}
	}
}
