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

#include "common.h"
#include "model_normals.h"

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

void kv6_init() {
	kv6_load(&model_playerdead,file_load("kv6/playerdead.kv6"),0.1F);
	kv6_load(&model_playerhead,file_load("kv6/playerhead.kv6"),0.1F);
	kv6_load(&model_playertorso,file_load("kv6/playertorso.kv6"),0.1F);
	kv6_load(&model_playertorsoc,file_load("kv6/playertorsoc.kv6"),0.1F);
	kv6_load(&model_playerarms,file_load("kv6/playerarms.kv6"),0.1F);
	kv6_load(&model_playerleg,file_load("kv6/playerleg.kv6"),0.1F);
	kv6_load(&model_playerlegc,file_load("kv6/playerlegc.kv6"),0.1F);

	kv6_load(&model_intel,file_load("kv6/intel.kv6"),0.2F);
	kv6_load(&model_tent,file_load("kv6/cp.kv6"),0.278F);

	kv6_load(&model_semi,file_load("kv6/semi.kv6"),0.05F);
	kv6_load(&model_smg,file_load("kv6/smg.kv6"),0.05F);
	kv6_load(&model_shotgun,file_load("kv6/shotgun.kv6"),0.05F);
	kv6_load(&model_spade,file_load("kv6/spade.kv6"),0.05F);
	kv6_load(&model_block,file_load("kv6/block.kv6"),0.05F);
	model_block.colorize = 1;
	kv6_load(&model_grenade,file_load("kv6/grenade.kv6"),0.05F);

	kv6_load(&model_semi_tracer,file_load("kv6/semitracer.kv6"),0.05F);
	kv6_load(&model_smg_tracer,file_load("kv6/smgtracer.kv6"),0.05F);
	kv6_load(&model_shotgun_tracer,file_load("kv6/shotguntracer.kv6"),0.05F);

	kv6_load(&model_semi_casing,file_load("kv6/semicasing.kv6"),0.0125F);
	kv6_load(&model_smg_casing,file_load("kv6/smgcasing.kv6"),0.0125F);
	kv6_load(&model_shotgun_casing,file_load("kv6/shotguncasing.kv6"),0.0125F);
}

void kv6_rebuild_all() {
	//only needed for models that contain team colors
	kv6_rebuild(&model_playerdead);
	kv6_rebuild(&model_playerhead);
	kv6_rebuild(&model_playertorso);
	kv6_rebuild(&model_playertorsoc);
	kv6_rebuild(&model_playerarms);
	kv6_rebuild(&model_playerleg);
	kv6_rebuild(&model_playerlegc);
	kv6_rebuild(&model_intel);
	kv6_rebuild(&model_tent);
}

void kv6_load(struct kv6_t* kv6, unsigned char* bytes, float scale) {
	kv6->colorize = 0;
	kv6->has_display_list = 0;
	kv6->scale = scale;
	int index = 0;
	if(buffer_read32(bytes,index)==0x6C78764B) { //"Kvxl"
		index += 4;
		kv6->xsiz = buffer_read32(bytes,index);
		index += 4;
		kv6->ysiz = buffer_read32(bytes,index);
		index += 4;
		kv6->zsiz = buffer_read32(bytes,index);
		index += 4;

		kv6->xpiv = buffer_readf(bytes,index);
		index += 4;
		kv6->ypiv = buffer_readf(bytes,index);
		index += 4;
		kv6->zpiv = kv6->zsiz-buffer_readf(bytes,index);
		index += 4;

		int blklen = buffer_read32(bytes,index);
		index += 4;
		unsigned int* blkdata_color = malloc(blklen*4);
		CHECK_ALLOCATION_ERROR(blkdata_color)
		unsigned short* blkdata_zpos = malloc(blklen*2);
		CHECK_ALLOCATION_ERROR(blkdata_zpos)
		unsigned char* blkdata_visfaces = malloc(blklen);
		CHECK_ALLOCATION_ERROR(blkdata_visfaces)
		unsigned char* blkdata_lighting = malloc(blklen);
		CHECK_ALLOCATION_ERROR(blkdata_lighting)
		for(int k=0;k<blklen;k++) {
			blkdata_color[k] = buffer_read32(bytes,index);
			index += 4;
			blkdata_zpos[k] = buffer_read16(bytes,index);
			index += 2;
			blkdata_visfaces[k] = buffer_read8(bytes,index++); //0x00zZyYxX
			blkdata_lighting[k] = buffer_read8(bytes,index++); //compressed normal vector (also referred to as lighting)
		}
		index += 4*kv6->xsiz;
		kv6->voxels = malloc(kv6->xsiz*kv6->ysiz*kv6->zsiz*sizeof(struct kv6_voxel));
		CHECK_ALLOCATION_ERROR(kv6->voxels)
		kv6->voxel_count = 0;
		for(int x=0;x<kv6->xsiz;x++) {
			for(int y=0;y<kv6->ysiz;y++) {
				int size = buffer_read16(bytes,index);
				index += 2;
				for(int z=0;z<size;z++) {
					struct kv6_voxel* voxel = &kv6->voxels[kv6->voxel_count];
					voxel->x = x;
					voxel->y = y;
					voxel->z = (kv6->zsiz-1)-blkdata_zpos[kv6->voxel_count];
					voxel->color = (blkdata_color[kv6->voxel_count]&0xFFFFFF)|(blkdata_lighting[kv6->voxel_count]<<24);
					voxel->visfaces = blkdata_visfaces[kv6->voxel_count];
					kv6->voxel_count++;
				}
			}
		}
		kv6->voxels = realloc(kv6->voxels,kv6->voxel_count*sizeof(struct kv6_voxel));
		CHECK_ALLOCATION_ERROR(kv6->voxels)

		free(blkdata_color);
		free(blkdata_zpos);
		free(blkdata_visfaces);
		free(blkdata_lighting);
	}
}

char kv6_intersection(struct kv6_t* kv6, Ray* r) {
	AABB bb;

	for(int k=0;k<8;k++) {
		float v[4] = {(kv6->xsiz*((k&1)>0)-kv6->xpiv)*kv6->scale,(kv6->zsiz*((k&2)>0)-kv6->zpiv)*kv6->scale,(kv6->ysiz*((k&4)>0)-kv6->ypiv)*kv6->scale,1.0F};
		matrix_vector(v);
		bb.min_x = (k==0)?v[0]:min(bb.min_x,v[0]);
		bb.min_y = (k==0)?v[1]:min(bb.min_y,v[1]);
		bb.min_z = (k==0)?v[2]:min(bb.min_z,v[2]);

		bb.max_x = (k==0)?v[0]:max(bb.max_x,v[0]);
		bb.max_y = (k==0)?v[1]:max(bb.max_y,v[1]);
		bb.max_z = (k==0)?v[2]:max(bb.max_z,v[2]);
	}

	return aabb_intersection_ray(&bb,r);
}

void kv6_rebuild(struct kv6_t* kv6) {
	if(kv6->has_display_list) {
		glx_displaylist_destroy(&kv6->display_list[0]);
		glx_displaylist_destroy(&kv6->display_list[1]);
		glx_displaylist_destroy(&kv6->display_list[2]);
		if(kv6->colorize) {
			free(kv6->vertices_final);
			free(kv6->colors_final);
			free(kv6->normals_final);
		}
		kv6->has_display_list = 0;
	}
}

void kv6_calclight(int x, int y, int z) {
	float f = 1.0F;
	if(x>=0 && y>=0 && z>=0)
		f = map_sunblock(x,y,z);
	float lambient[4] = {0.5F*f,0.5F*f,0.5F*f,1.0F};
	float ldiffuse[4] = {0.5F*f,0.5F*f,0.5F*f,1.0F};
	glLightfv(GL_LIGHT0,GL_AMBIENT,lambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,ldiffuse);
}

static int kv6_program = -1;
void kv6_render(struct kv6_t* kv6, unsigned char team) {
	if(!kv6->has_display_list) {
		int size = kv6->voxel_count*6;

		#ifdef OPENGL_ES
		kv6->vertices_final = malloc(size*3*6*sizeof(float));
		CHECK_ALLOCATION_ERROR(kv6->vertices_final)
		kv6->colors_final = malloc(size*4*6*sizeof(unsigned char)*2);
		CHECK_ALLOCATION_ERROR(kv6->colors_final)
		kv6->normals_final = malloc(size*3*6*sizeof(unsigned char));
		CHECK_ALLOCATION_ERROR(kv6->normals_final)
		#else
		kv6->vertices_final = malloc(size*3*4*sizeof(float));
		CHECK_ALLOCATION_ERROR(kv6->vertices_final)
		kv6->colors_final = malloc(size*4*4*sizeof(unsigned char)*2);
		CHECK_ALLOCATION_ERROR(kv6->colors_final)
		kv6->normals_final = malloc(size*3*4*sizeof(unsigned char));
		CHECK_ALLOCATION_ERROR(kv6->normals_final)
		#endif

		if(!kv6->colorize) {
			glx_displaylist_create(&kv6->display_list[0]);
			glx_displaylist_create(&kv6->display_list[1]);
			glx_displaylist_create(&kv6->display_list[2]);
		}

		int v,c,n;
		for(int t=0;t<3;t++) {
			v = c = n = 0;
			for(int k=0;k<kv6->voxel_count;k++) {
				int x = kv6->voxels[k].x;
				int y = kv6->voxels[k].y;
				int z = kv6->voxels[k].z;
				int b = kv6->voxels[k].color & 0xFF;
				int g = (kv6->voxels[k].color>>8) & 0xFF;
				int r = (kv6->voxels[k].color>>16) & 0xFF;
				int a = (kv6->voxels[k].color>>24) & 0xFF;
				if(r==0 && g==0 && b==0) {
					switch(t) {
						case TEAM_1:
							r = gamestate.team_1.red*0.75F;
							g = gamestate.team_1.green*0.75F;
							b = gamestate.team_1.blue*0.75F;
							break;
						case TEAM_2:
							r = gamestate.team_2.red*0.75F;
							g = gamestate.team_2.green*0.75F;
							b = gamestate.team_2.blue*0.75F;
							break;
						default:
							r = g = b = 0;
					}
				}

				float p[3] = {(x-kv6->xpiv)*kv6->scale,(z-kv6->zpiv)*kv6->scale,(y-kv6->ypiv)*kv6->scale};

				int i = 0;
				float alpha[12];

				//negative y
				if(kv6->voxels[k].visfaces&16) {
					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					#ifdef OPENGL_ES
					alpha[i++] = 1.0F;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					#endif

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];
					alpha[i++] = 1.0F;
				}

				//positive y
				if(kv6->voxels[k].visfaces&32) {
					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					#ifdef OPENGL_ES
					alpha[i++] = 0.6F;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					#endif

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					alpha[i++] = 0.6F;
				}

				//negative z
				if(kv6->voxels[k].visfaces&4) {
					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];

					#ifdef OPENGL_ES
					alpha[i++] = 0.95F;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];
					#endif

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];
					alpha[i++] = 0.95F;
				}

				//positive z
				if(kv6->voxels[k].visfaces&8) {
					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					#ifdef OPENGL_ES
					alpha[i++] = 0.9F;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					#endif

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					alpha[i++] = 0.9F;
				}

				//negative x
				if(kv6->voxels[k].visfaces&1) {
					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					#ifdef OPENGL_ES
					alpha[i++] = 0.85F;

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					#endif

					kv6->vertices_final[v++] = p[0];
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];
					alpha[i++] = 0.85F;
				}

				//positive x
				if(kv6->voxels[k].visfaces&2) {
					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;

					#ifdef OPENGL_ES
					alpha[i++] = 0.8F;

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2];

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1]+kv6->scale;
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					#endif

					kv6->vertices_final[v++] = p[0]+kv6->scale;
					kv6->vertices_final[v++] = p[1];
					kv6->vertices_final[v++] = p[2]+kv6->scale;
					alpha[i++] = 0.8F;
				}

				#ifdef OPENGL_ES
				for(int k=0;k<i*3;k++) {
					kv6->colors_final[c++] = r*alpha[k/3];
					kv6->colors_final[c++] = g*alpha[k/3];
					kv6->colors_final[c++] = b*alpha[k/3];
					kv6->colors_final[c++] = 255;
				#else
				for(int k=0;k<i*4;k++) {
					kv6->colors_final[c++] = r*alpha[k/4];
					kv6->colors_final[c++] = g*alpha[k/4];
					kv6->colors_final[c++] = b*alpha[k/4];
					kv6->colors_final[c++] = 255;
				#endif
					kv6->normals_final[n++] = kv6_normals[a][0]*128;
					kv6->normals_final[n++] = -kv6_normals[a][2]*128;
					kv6->normals_final[n++] = kv6_normals[a][1]*128;
				}
			}

			if(!kv6->colorize)
				glx_displaylist_update(&kv6->display_list[t],v/3,GLX_DISPLAYLIST_ENHANCED,kv6->colors_final,kv6->vertices_final,kv6->normals_final);
		}

		if(!kv6->colorize) {
			free(kv6->vertices_final);
			free(kv6->colors_final);
			free(kv6->normals_final);
		} else {
			memcpy(kv6->colors_final+c*sizeof(unsigned char),kv6->colors_final,c*sizeof(unsigned char));
			kv6->size = v/3;
		}

		kv6->has_display_list = 1;
	} else {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		#ifndef OPENGL_ES
		glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
		#endif
		glEnable(GL_NORMALIZE);
		if(!kv6->colorize) {
			glx_displaylist_draw(&kv6->display_list[team%3],GLX_DISPLAYLIST_ENHANCED);
		} else {
			for(int k=0;k<kv6->size*4;k+=4) {
				kv6->colors_final[k+0] = min(((float)kv6->colors_final[k+0+kv6->size*4*sizeof(unsigned char)])/0.4335F*kv6->red,255);
				kv6->colors_final[k+1] = min(((float)kv6->colors_final[k+1+kv6->size*4*sizeof(unsigned char)])/0.4335F*kv6->green,255);
				kv6->colors_final[k+2] = min(((float)kv6->colors_final[k+2+kv6->size*4*sizeof(unsigned char)])/0.4335F*kv6->blue,255);
			}
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,kv6->vertices_final);
			glColorPointer(4,GL_UNSIGNED_BYTE,0,kv6->colors_final);
			glNormalPointer(GL_BYTE,0,kv6->normals_final);
			#ifdef OPENGL_ES
			glDrawArrays(GL_TRIANGLES,0,kv6->size);
			#else
			glDrawArrays(GL_QUADS,0,kv6->size);
			#endif
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
		}
		glDisable(GL_NORMALIZE);
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}



	//TODO: render like in voxlap

	/*if(!kv6->has_display_list) {
		int size = kv6->voxel_count*6;
		kv6->vertices_final = malloc(size*12*sizeof(float));
		kv6->colors_final = malloc(size*12*sizeof(unsigned char)*3);
		kv6->normals_final = malloc(size*12*sizeof(unsigned char));
		kv6->has_display_list = 1;
		if(kv6_program<0) {
			kv6_program = glx_vertex_shader("uniform float size;\n" \
											"uniform vec3 fog;\n" \
											"uniform vec3 camera;\n" \
											"uniform mat4 model;\n" \
											"void main() {\n" \
											"	gl_Position = gl_ModelViewProjectionMatrix*gl_Vertex;\n" \
											"	float dist = length((model*gl_Vertex).xz-camera.xz);\n" \
											"	vec3 N = normalize(model*vec4(gl_Normal,0)).xyz;\n" \
											"	vec3 L = normalize(vec3(0,-1,1));\n" \
											"	float d = clamp(dot(N,L),0.0,1.0)*0.5+0.5;\n" \
											"	gl_FrontColor = mix(vec4(d,d,d,1.0)*gl_Color,vec4(fog,1.0),min(dist/128.0,1.0));\n" \
											"	gl_PointSize = size/gl_Position.w;\n" \
											"}\n",NULL);
		}
	}

	int k = 0;

	for(int abc=0;abc<kv6->voxel_count;abc++) {
		int x = kv6->voxels[abc].x;
		int y = kv6->voxels[abc].y;
		int z = kv6->voxels[abc].z;
		int b = kv6->voxels[abc].color & 0xFF;
		int g = (kv6->voxels[abc].color>>8) & 0xFF;
		int r = (kv6->voxels[abc].color>>16) & 0xFF;
		int a = (kv6->voxels[abc].color>>24) & 0xFF;
		if(r==0 && g==0 && b==0) {
			switch(team) {
				case TEAM_1:
					r = gamestate.team_1.red*0.75F;
					g = gamestate.team_1.green*0.75F;
					b = gamestate.team_1.blue*0.75F;
					break;
				case TEAM_2:
					r = gamestate.team_2.red*0.75F;
					g = gamestate.team_2.green*0.75F;
					b = gamestate.team_2.blue*0.75F;
					break;
				default:
					r = g = b = 0;
			}
		}

		float v[3] = {(x-kv6->xpiv)*kv6->scale,(z-kv6->zpiv)*kv6->scale,(y-kv6->ypiv)*kv6->scale};
		float n[3] = {kv6_normals[a][0],-kv6_normals[a][2],kv6_normals[a][1]};

		kv6->colors_final[k] = r;
		kv6->normals_final[k] = n[0]*128;
		kv6->vertices_final[k++] = v[0]+kv6->scale*0.5F;
		kv6->colors_final[k] = g;
		kv6->normals_final[k] = n[1]*128;
		kv6->vertices_final[k++] = v[1]+kv6->scale*0.5F;
		kv6->colors_final[k] = b;
		kv6->normals_final[k] = n[2]*128;
		kv6->vertices_final[k++] = v[2]+kv6->scale*0.5F;
	}

	float heightOfNearPlane = (float)settings.window_height/(2.0F*tan(camera_fov_scaled()*1.570796F/180.0F));

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3,GL_FLOAT,0,kv6->vertices_final);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,kv6->colors_final);
	glNormalPointer(GL_BYTE,0,kv6->normals_final);
	glEnable(GL_PROGRAM_POINT_SIZE);
	int i = glx_fog;
	if(i)
		glx_disable_sphericalfog();
	glUseProgram(kv6_program);
	glUniform1f(glGetUniformLocation(kv6_program,"size"),1.414F*kv6->scale*heightOfNearPlane);
	glUniform3f(glGetUniformLocation(kv6_program,"fog"),fog_color[0],fog_color[1],fog_color[2]);
	glUniform3f(glGetUniformLocation(kv6_program,"camera"),camera_x,camera_y,camera_z);
	glUniformMatrix4fv(glGetUniformLocation(kv6_program,"model"),1,0,matrix_model);
	glDrawArrays(GL_POINTS,0,k/3);
	glUseProgram(0);
	if(i)
		glx_enable_sphericalfog();
	glDisable(GL_PROGRAM_POINT_SIZE);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);*/
}
