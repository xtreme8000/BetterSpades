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

unsigned int* map_colors;
int map_size_x = 512;
int map_size_y = 64;
int map_size_z = 512;

float fog_color[4] = {0.5F,0.9098F,1.0F,1.0F};

float map_sun[4];

unsigned char* map_minimap;

struct DamagedVoxel map_damaged_voxels[8] = {0};

int map_object_visible(float x, float y, float z) {
	return !(x<=0.0F && z<=0.0F);
}

int map_damage(int x, int y, int z, int damage) {
	for(int k=0;k<8;k++) {
		if(window_time()-map_damaged_voxels[k].timer<=10.0F && map_damaged_voxels[k].x==x && map_damaged_voxels[k].y==y && map_damaged_voxels[k].z==z) {
			map_damaged_voxels[k].damage = min(damage+map_damaged_voxels[k].damage,100);
			map_damaged_voxels[k].timer = window_time();
			return map_damaged_voxels[k].damage;
		}
	}
	int r = 0;
	for(int k=0;k<8;k++) {
		if(window_time()-map_damaged_voxels[k].timer>10.0F) {
			r = k;
			break;
		}
	}
	map_damaged_voxels[r].x = x;
	map_damaged_voxels[r].y = y;
	map_damaged_voxels[r].z = z;
	map_damaged_voxels[r].damage = damage;
	map_damaged_voxels[r].timer = window_time();
	return damage;
}

int map_damage_get(int x, int y, int z) {
	for(int k=0;k<8;k++) {
		if(window_time()-map_damaged_voxels[k].timer<=10.0F
		&& map_damaged_voxels[k].x==x
		&& map_damaged_voxels[k].y==y
		&& map_damaged_voxels[k].z==z) {
			return map_damaged_voxels[k].damage;
		}
	}
	return 0;
}

static void push_float3(float* buffer, int* index, float x, float y, float z) {
	buffer[(*index)++] = x;
	buffer[(*index)++] = y;
	buffer[(*index)++] = z;
}

static void push_char4(unsigned char* buffer, int* index, int r, int g, int b, int a) {
	buffer[(*index)++] = r;
	buffer[(*index)++] = g;
	buffer[(*index)++] = b;
	buffer[(*index)++] = a;
}

static void push_char46(unsigned char* buffer, int* index, int r, int g, int b, int a) {
	for(int k=0;k<6;k++) {
		push_char4(buffer,index,r,g,b,a);
	}
}

void map_damaged_voxels_render() {
	matrix_identity();
	matrix_upload();
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(0.0F,-100.0F);
	glEnable(GL_BLEND);
	for(int k=0;k<8;k++) {
		if(map_get(map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z)==0xFFFFFFFF) {
			map_damaged_voxels[k].timer = 0;
		} else {
			if(window_time()-map_damaged_voxels[k].timer<=10.0F) {
				float vertices[6*18];
				unsigned char colors[6*24];
				int color_index = 0, vertex_index = 0;

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z);

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);

				push_char46(colors,&color_index,0,0,0,map_damaged_voxels[k].damage*1.9125F);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y+1,map_damaged_voxels[k].z+1);
				push_float3(vertices,&vertex_index,map_damaged_voxels[k].x+1,map_damaged_voxels[k].y,map_damaged_voxels[k].z+1);

				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);
				glVertexPointer(3,GL_FLOAT,0,vertices);
				glColorPointer(4,GL_UNSIGNED_BYTE,0,colors);
				glDrawArrays(GL_TRIANGLES,0,vertex_index/3);
				glDisableClientState(GL_COLOR_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
			}
		}
	}
	glDisable(GL_BLEND);
	glPolygonOffset(0.0F,0.0F);
	glDisable(GL_POLYGON_OFFSET_FILL);
}

struct voxel {
	int x,y,z;
	char processed;
};

static char stack_contains(struct voxel* stack2, int len, int x, int y, int z) {
	for(int k=0;k<len;k++)
		if(stack2[k].x==x && stack2[k].y==y && stack2[k].z==z)
			return 1;
	return 0;
}

static struct {
	struct voxel* voxels;
	unsigned int* voxels_color;
	int voxel_count;
	struct Velocity v;
	struct Position p;
	struct Position p2;
	struct Orientation o;
	char used, rotation, has_displaylist;
	struct glx_displaylist displaylist;
} map_collapsing_structures[32] = {0};

static struct voxel* stack = NULL;
static int stack_size;

static void map_update_physics_sub(int x, int y, int z) {
	//TODO: remove calls to malloc and realloc
	if(!stack) {
		stack_size = 4096;
		stack = malloc(stack_size*sizeof(struct voxel));
		CHECK_ALLOCATION_ERROR(stack)
	}
	int stack_depth = 1;
	stack[0].x = x;
	stack[0].y = y;
	stack[0].z = z;
	stack[0].processed = 1;

	while(1) { //find all connected blocks
		if(map_get(x,y-1,z)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x,y-1,z)) {
			if(y<=2) {
				return;
			}
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x;
			stack[stack_depth].y = y-1;
			stack[stack_depth++].z = z;
			if(stack_depth==stack_size) {
				stack_size += 4096;
				stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}
		if(map_get(x,y+1,z)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x,y+1,z)) {
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x;
			stack[stack_depth].y = y+1;
			stack[stack_depth++].z = z;
			if(stack_depth==stack_size) {
				stack_size += 4096;
                stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}
		if(map_get(x+1,y,z)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x+1,y,z)) {
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x+1;
			stack[stack_depth].y = y;
			stack[stack_depth++].z = z;
			if(stack_depth==stack_size) {
				stack_size += 4096;
				stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}
		if(map_get(x-1,y,z)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x-1,y,z)) {
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x-1;
			stack[stack_depth].y = y;
			stack[stack_depth++].z = z;
			if(stack_depth==stack_size) {
				stack_size += 4096;
				stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}
		if(map_get(x,y,z+1)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x,y,z+1)) {
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x;
			stack[stack_depth].y = y;
			stack[stack_depth++].z = z+1;
			if(stack_depth==stack_size) {
				stack_size += 4096;
				stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}
		if(map_get(x,y,z-1)!=0xFFFFFFFF && !stack_contains(stack,stack_depth,x,y,z-1)) {
			stack[stack_depth].processed = 0;
			stack[stack_depth].x = x;
			stack[stack_depth].y = y;
			stack[stack_depth++].z = z-1;
			if(stack_depth==stack_size) {
				stack_size += 4096;
				stack = realloc(stack,stack_size*sizeof(struct voxel));
        		CHECK_ALLOCATION_ERROR(stack)
			}
		}


		//find the next best block to work on
		//first: lower y coord, second: equal y coord, last: everything else
		int k, level = -1, level_best = 0;
		for(k=0;k<stack_depth;k++) {
			if(!stack[k].processed) {
				if(stack[k].y==y-1 && stack[k].x==x && stack[k].z==z) {
					level = 4;
					level_best = k;
					break;
				}
				if(stack[k].y<y && level<3) {
					level = 3;
					level_best = k;
					continue;
				}
				if(stack[k].y==y && level<2) {
					level = 2;
					level_best = k;
					continue;
				}
				if(level<1) {
					level = 1;
					level_best = k;
					continue;
				}
			}
		}
		if(level<0) { //all connected blocks processed
			break;
		} else {
			x = stack[level_best].x;
			y = stack[level_best].y;
			z = stack[level_best].z;
			stack[level_best].processed = 1;
		}
	}

	if(stack_depth==0) {
		return;
	}

	for(int k=0;k<32;k++) {
		if(!map_collapsing_structures[k].used) {
			float px = 0.0F,py = 0.0F,pz = 0.0F;
			map_collapsing_structures[k].voxels_color = malloc(stack_depth*sizeof(unsigned int));
			CHECK_ALLOCATION_ERROR(map_collapsing_structures[k].voxels_color)
			for(int i=0;i<stack_depth;i++) {
				px += stack[i].x+0.5F;
				py += stack[i].y+0.5F;
				pz += stack[i].z+0.5F;
				map_collapsing_structures[k].voxels_color[i] = map_get(stack[i].x,stack[i].y,stack[i].z);
				map_set(stack[i].x,stack[i].y,stack[i].z,0xFFFFFFFF);
			}

			px /= (float)stack_depth;
			py /= (float)stack_depth;
			pz /= (float)stack_depth;

			sound_create(NULL,SOUND_WORLD,&sound_debris,px,py,pz);

			map_collapsing_structures[k].used = 1;
			map_collapsing_structures[k].voxels = stack;
			stack = NULL;
			map_collapsing_structures[k].v = (struct Velocity) {0,0,0};
			map_collapsing_structures[k].o = (struct Orientation) {0,0,0};
			map_collapsing_structures[k].p = (struct Position) {px,py,pz};
			map_collapsing_structures[k].p2 = (struct Position) {px,py,pz};
			map_collapsing_structures[k].rotation = rand()&3;
			map_collapsing_structures[k].voxel_count = stack_depth;
			map_collapsing_structures[k].has_displaylist = 0;
			return;
		}
	}
}

void map_collapsing_render(float dt) {
	glEnable(GL_BLEND);
	for(int k=0;k<32;k++) {
		if(map_collapsing_structures[k].used) {
			map_collapsing_structures[k].v.y -= dt;
			char hit_floor = 0;
			if(map_get(map_collapsing_structures[k].p.x+map_collapsing_structures[k].v.x*dt*32.0F,
					   map_collapsing_structures[k].p.y+map_collapsing_structures[k].v.y*dt*32.0F,
					   map_collapsing_structures[k].p.z+map_collapsing_structures[k].v.z*dt*32.0F)==0xFFFFFFFF) {
				map_collapsing_structures[k].p.x += map_collapsing_structures[k].v.x*dt*32.0F;
				map_collapsing_structures[k].p.y += map_collapsing_structures[k].v.y*dt*32.0F;
				map_collapsing_structures[k].p.z += map_collapsing_structures[k].v.z*dt*32.0F;
			} else {
				map_collapsing_structures[k].v.x *= 0.7F;
				map_collapsing_structures[k].v.y *= -0.7F;
				map_collapsing_structures[k].v.z *= 0.7F;
				map_collapsing_structures[k].rotation++;
				map_collapsing_structures[k].rotation &= 3;
				sound_create(NULL,SOUND_WORLD,&sound_bounce,map_collapsing_structures[k].p.x,map_collapsing_structures[k].p.y,map_collapsing_structures[k].p.z);
				hit_floor = 1;
			}
			matrix_push();
			matrix_identity();
			matrix_translate(map_collapsing_structures[k].p.x,map_collapsing_structures[k].p.y,map_collapsing_structures[k].p.z);
			map_collapsing_structures[k].o.x += ((map_collapsing_structures[k].rotation&1)?1.0F:-1.0F)*dt*75.0F;
			map_collapsing_structures[k].o.y += ((map_collapsing_structures[k].rotation&2)?1.0F:-1.0F)*dt*75.0F;
			matrix_rotate(map_collapsing_structures[k].o.x,1.0F,0.0F,0.0F);
			matrix_rotate(map_collapsing_structures[k].o.y,0.0F,1.0F,0.0F);
			matrix_upload();
			if(!map_collapsing_structures[k].has_displaylist) {
				map_collapsing_structures[k].has_displaylist = 1;
				glx_displaylist_create(&map_collapsing_structures[k].displaylist);

				#ifdef OPENGL_ES
					float* vertices = malloc(map_collapsing_structures[k].voxel_count*6*6*3*sizeof(float)); //max space needed
					CHECK_ALLOCATION_ERROR(vertices)
					unsigned char* colors = malloc(map_collapsing_structures[k].voxel_count*6*6*4*sizeof(unsigned char));
					CHECK_ALLOCATION_ERROR(colors)
				#else
					float* vertices = malloc(map_collapsing_structures[k].voxel_count*6*4*3*sizeof(float)); //same
					CHECK_ALLOCATION_ERROR(vertices)
					unsigned char* colors = malloc(map_collapsing_structures[k].voxel_count*6*4*4*sizeof(unsigned char));
					CHECK_ALLOCATION_ERROR(colors)
				#endif

				int vertex_index = 0;
				int color_index = 0;

				for(int i=0;i<map_collapsing_structures[k].voxel_count;i++) {
					float x = map_collapsing_structures[k].voxels[i].x-map_collapsing_structures[k].p2.x;
					float y = map_collapsing_structures[k].voxels[i].y-map_collapsing_structures[k].p2.y;
					float z = map_collapsing_structures[k].voxels[i].z-map_collapsing_structures[k].p2.z;

					int x2 = map_collapsing_structures[k].voxels[i].x;
					int y2 = map_collapsing_structures[k].voxels[i].y;
					int z2 = map_collapsing_structures[k].voxels[i].z;

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2,y2-1,z2)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i])*0.5F,
								green(map_collapsing_structures[k].voxels_color[i])*0.5F,
								blue(map_collapsing_structures[k].voxels_color[i])*0.5F,0xCC);
						}
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x+1,y,z);
						push_float3(vertices,&vertex_index,x+1,y,z+1);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x+1,y,z+1);
						#endif
						push_float3(vertices,&vertex_index,x,y,z+1);
					}

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2,y2+1,z2)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i]),
								green(map_collapsing_structures[k].voxels_color[i]),
								blue(map_collapsing_structures[k].voxels_color[i]),0xCC);
						}
						push_float3(vertices,&vertex_index,x,y+1,z);
						push_float3(vertices,&vertex_index,x,y+1,z+1);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x,y+1,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#endif
						push_float3(vertices,&vertex_index,x+1,y+1,z);
					}

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2,y2,z2-1)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i])*0.7F,
								green(map_collapsing_structures[k].voxels_color[i])*0.7F,
								blue(map_collapsing_structures[k].voxels_color[i])*0.7F,0xCC);
						}
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x,y+1,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z);
						#endif
						push_float3(vertices,&vertex_index,x+1,y,z);
					}

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2,y2,z2+1)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i])*0.6F,
								green(map_collapsing_structures[k].voxels_color[i])*0.6F,
								blue(map_collapsing_structures[k].voxels_color[i])*0.6F,0xCC);
						}
						push_float3(vertices,&vertex_index,x,y,z+1);
						push_float3(vertices,&vertex_index,x+1,y,z+1);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x,y,z+1);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#endif
						push_float3(vertices,&vertex_index,x,y+1,z+1);
					}

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2-1,y2,z2)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i])*0.9F,
								green(map_collapsing_structures[k].voxels_color[i])*0.9F,
								blue(map_collapsing_structures[k].voxels_color[i])*0.9F,0xCC);
						}
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x,y,z+1);
						push_float3(vertices,&vertex_index,x,y+1,z+1);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x,y,z);
						push_float3(vertices,&vertex_index,x,y+1,z+1);
						#endif
						push_float3(vertices,&vertex_index,x,y+1,z);
					}

					if(!stack_contains(map_collapsing_structures[k].voxels,map_collapsing_structures[k].voxel_count,x2+1,y2,z2)) {
						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							push_char4(colors,&color_index,
								red(map_collapsing_structures[k].voxels_color[i])*0.8F,
								green(map_collapsing_structures[k].voxels_color[i])*0.8F,
								blue(map_collapsing_structures[k].voxels_color[i])*0.8F,0xCC);
						}
						push_float3(vertices,&vertex_index,x+1,y,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#ifdef OPENGL_ES
						push_float3(vertices,&vertex_index,x+1,y,z);
						push_float3(vertices,&vertex_index,x+1,y+1,z+1);
						#endif
						push_float3(vertices,&vertex_index,x+1,y,z+1);
					}
				}
				glx_displaylist_update(&map_collapsing_structures[k].displaylist,vertex_index/3,GLX_DISPLAYLIST_ENHANCED,colors,vertices,NULL);
				free(colors);
				free(vertices);
			}
			glx_displaylist_draw(&map_collapsing_structures[k].displaylist,GLX_DISPLAYLIST_ENHANCED);

			if(absf(map_collapsing_structures[k].v.y)<0.1F && hit_floor) {
				for(int i=0;i<map_collapsing_structures[k].voxel_count;i++) {
					float v[4] = {map_collapsing_structures[k].voxels[i].x-map_collapsing_structures[k].p2.x+0.5F,
								  map_collapsing_structures[k].voxels[i].y-map_collapsing_structures[k].p2.y+0.5F,
								  map_collapsing_structures[k].voxels[i].z-map_collapsing_structures[k].p2.z+0.5F,
								  1.0F};
					matrix_vector(v);
					particle_create(map_collapsing_structures[k].voxels_color[i],v[0],v[1],v[2],2.5F,1.0F,2,0.25F,0.4F);
				}
				free(map_collapsing_structures[k].voxels);
				free(map_collapsing_structures[k].voxels_color);

				glx_displaylist_destroy(&map_collapsing_structures[k].displaylist);
				map_collapsing_structures[k].used = 0;
			}

			matrix_pop();
		}
	}
	glDisable(GL_BLEND);
}

void map_update_physics(int x, int y, int z) {
	if(map_get(x+1,y,z)!=0xFFFFFFFF)
		map_update_physics_sub(x+1,y,z);
	if(map_get(x-1,y,z)!=0xFFFFFFFF)
		map_update_physics_sub(x-1,y,z);
	if(map_get(x,y,z+1)!=0xFFFFFFFF)
		map_update_physics_sub(x,y,z+1);
	if(map_get(x,y,z-1)!=0xFFFFFFFF)
		map_update_physics_sub(x,y,z-1);
	if(map_get(x,y-1,z)!=0xFFFFFFFF)
		map_update_physics_sub(x,y-1,z);
	if(map_get(x,y+1,z)!=0xFFFFFFFF)
		map_update_physics_sub(x,y+1,z);
}

//see this for details: https://github.com/infogulch/pyspades/blob/protocol075/pyspades/vxl_c.cpp#L380
float map_sunblock(int x, int y, int z) {
	int dec = 18;
	int i = 127;

	while(dec && y<64) {
		if((map_get(x,++y,--z)&0xFFFFFFFF)!=0xFFFFFFFF)
			i -= dec;
		dec -= 2;
	}
	return (float)i/127.0F;
}

int map_isair(int x, int y, int z) {
	return map_get(x,y,z)==0xFFFFFFFF;
}

unsigned int map_get(int x, int y, int z) {
	if(x<0 || y<0 || z<0 || x>=map_size_x || y>=map_size_y || z>=map_size_z)
		return 0xFFFFFFFF;

	pthread_rwlock_rdlock(&chunk_map_locks[x+z*map_size_x]);
	unsigned int ret = map_colors[x+(y*map_size_z+z)*map_size_x];
	pthread_rwlock_unlock(&chunk_map_locks[x+z*map_size_x]);
	return ret;
}

unsigned int map_get_unblocked(int x, int y, int z) {
	if(x<0 || y<0 || z<0 || x>=map_size_x || y>=map_size_y || z>=map_size_z)
		return 0xFFFFFFFF;

	return map_colors[x+(y*map_size_z+z)*map_size_x];
}

void map_set(int x, int y, int z, unsigned int color) {
	if(x<0 || y<0 || z<0 || x>=map_size_x || y>=map_size_y || z>=map_size_z)
		return;

	/*int err = pthread_rwlock_trywrlock(&chunk_map_lock);
	if(err!=0) {
		log_debug("pthread: %i",err);
		return;
	}*/
	pthread_rwlock_wrlock(&chunk_map_locks[x+z*map_size_x]);
	map_colors[x+(y*map_size_z+z)*map_size_x] = color;
	pthread_rwlock_unlock(&chunk_map_locks[x+z*map_size_x]);

	chunk_block_update(x,y,z);

	unsigned char x_off = x%CHUNK_SIZE;
	unsigned char z_off = z%CHUNK_SIZE;

	if(x>0 && x_off==0)
		chunk_block_update(x-1,y,z);
	if(z>0 && z_off==0)
		chunk_block_update(x,y,z-1);
	if(x<map_size_x-1 && x_off==CHUNK_SIZE-1)
		chunk_block_update(x+1,y,z);
	if(z<map_size_z-1 && z_off==CHUNK_SIZE-1)
		chunk_block_update(x,y,z+1);
	if(x>0 && z>0 && x_off==0 && z_off==0)
		chunk_block_update(x-1,y,z-1);
	if(x<map_size_x-1 && z<map_size_z-1 && x_off==CHUNK_SIZE-1 && z_off==CHUNK_SIZE-1)
		chunk_block_update(x+1,y,z+1);
	if(x>0 && z<map_size_z-1 && x_off==0 && z_off==CHUNK_SIZE-1)
		chunk_block_update(x-1,y,z+1);
	if(x<map_size_x-1 && z>0 && x_off==CHUNK_SIZE-1 && z_off==0)
		chunk_block_update(x+1,y,z-1);

	if(x==0)
		chunk_block_update(map_size_x-1,y,z);
	if(x==map_size_x-1)
		chunk_block_update(0,y,z);
	if(z==0)
		chunk_block_update(x,y,map_size_z-1);
	if(z==map_size_z-1)
		chunk_block_update(x,y,0);
}

void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned int* map) {
	if(x<0 || y<0 || z<0 || x>=map_size_x || y>=map_size_z || z>=map_size_y)
		return;

	map[x+((map_size_y-1-z)*map_size_z+y)*map_size_x] = t;
}

void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned int* map) {
	if(x<0 || y<0 || z<0 || x>=map_size_x || y>=map_size_z || z>=map_size_y)
		return;
	unsigned char r = t & 255;
	unsigned char g = (t>>8) & 255;
	unsigned char b = (t>>16) & 255;
	map[x+((map_size_y-1-z)*map_size_z+y)*map_size_x] = (r<<16)|(g<<8)|b;
}

//Copyright (c) Mathias Kaerlev 2011-2012 (but might be original code by Ben himself)
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array) {
	struct Point c, d;
	long ixi, iyi, izi, dx, dy, dz, dxi, dyi, dzi;
	int count = 0;

	//Note: positions MUST be rounded towards -inf
	c.x = x1;
	c.y = y1;
	c.z = z1;

	d.x = x2 - x1;
	d.y = y2 - y1;
	d.z = z2 - z1;

	if (d.x < 0) ixi = -1;
	else ixi = 1;
	if (d.y < 0) iyi = -1;
	else iyi = 1;
	if (d.z < 0) izi = -1;
	else izi = 1;

	if ((abs(d.x) >= abs(d.y)) && (abs(d.x) >= abs(d.z))) {
		dxi = 1024; dx = 512;
		dyi = (long)(!d.y ? 0x3fffffff/512 : abs(d.x*1024/d.y));
		dy = dyi/2;
		dzi = (long)(!d.z ? 0x3fffffff/512 : abs(d.x*1024/d.z));
		dz = dzi/2;
	} else if (abs(d.y) >= abs(d.z)) {
		dyi = 1024; dy = 512;
		dxi = (long)(!d.x ? 0x3fffffff/512 : abs(d.y*1024/d.x));
		dx = dxi/2;
		dzi = (long)(!d.z ? 0x3fffffff/512 : abs(d.y*1024/d.z));
		dz = dzi/2;
	} else {
		dzi = 1024; dz = 512;
		dxi = (long)(!d.x ? 0x3fffffff/512 : abs(d.z*1024/d.x));
		dx = dxi/2;
		dyi = (long)(!d.y ? 0x3fffffff/512 : abs(d.z*1024/d.y));
		dy = dyi/2;
	}
	if (ixi >= 0) dx = dxi-dx;
	if (iyi >= 0) dy = dyi-dy;
	if (izi >= 0) dz = dzi-dz;

	while(1) {
		if(cube_array!=NULL)
			cube_array[count] = c;

		if(count++==63)
			return count;

		if(c.x==x2 && c.y==y2 && c.z==z2)
			return count;

		if (dz<=dx && dz<=dy) {
			c.z += izi;
			if (c.z<0 || c.z>=64)
				return count;
			dz += dzi;
		} else {
			if(dx < dy) {
				c.x += ixi;
				if ((unsigned long)c.x>=512)
					return count;
				dx += dxi;
			} else {
				c.y += iyi;
				if((unsigned long)c.y>=512)
					return count;
				dy += dyi;
			}
		}
	}
}

static int lerp(int a, int b, int amt) { //amt from 0 to 8
	return a + (b-a)*amt/8;
}

static int dirt_color_table[] = {
	0x506050, 0x605848, 0x705040,
	0x804838, 0x704030, 0x603828,
	0x503020, 0x402818, 0x302010
};

//thanks to BR_ for deobfuscating this method
int map_dirt_color(int x, int y, int z) {
	int vertical_slice = (map_size_y-1-y)/8;
	int lerp_amt = (map_size_y-1-y)%8;
	int dirt_base_color = dirt_color_table[vertical_slice];
	int dirt_next_color = dirt_color_table[vertical_slice+1];

	int red   = lerp(dirt_base_color&0xFF0000,dirt_next_color&0xFF0000,lerp_amt)>>16;
	int green = lerp(dirt_base_color&0x00FF00,dirt_next_color&0x00FF00,lerp_amt)>>8;
	int blue  = lerp(dirt_base_color&0x0000FF,dirt_next_color&0x0000FF,lerp_amt);

	int rng = ms_rand()%8;
	red   += 4*abs((x%8)-4)+rng;
	green += 4*abs((z%8)-4)+rng;
	blue  += 4*abs(((map_size_y-1-y)%8)-4)+rng;

	return rgb(red,green,blue);
}

static int gkrand = 0;
int map_placedblock_color(int color) {
	color = color | 0x7F000000;
	gkrand = 0x1A4E86D*gkrand+1;
	return color^(gkrand&0x70707);
}

void map_vxl_load(unsigned char* v, unsigned int* map) {
	for(int k=0;k<map_size_x*map_size_z;k++)
		pthread_rwlock_wrlock(&chunk_map_locks[k]);
	for(int y=0;y<map_size_z;y++) {
		for(int x=0;x<map_size_x;x++) {
			int z;
			for(z=0;z<map_size_y;z++) {
				map_vxl_setgeom(x,y,z,map_dirt_color(x,map_size_y-1-z,y),map);
			}
			z = 0;
			while(1) {
				unsigned int *color;
				int i;
				int number_4byte_chunks = v[0];
				int top_color_start = v[1];
				int top_color_end   = v[2]; // inclusive
				int bottom_color_start;
				int bottom_color_end; // exclusive
				int len_top;
				int len_bottom;

				for(i=z;i<top_color_start;i++) {
					map_vxl_setgeom(x,y,i,0xFFFFFFFF,map);
				}

				color = (unsigned int*) (v+4);
				for(z=top_color_start;z<=top_color_end;z++) {
					map_vxl_setcolor(x,y,z,*color++,map);
				}

				len_bottom = top_color_end - top_color_start + 1;

				// check for end of data marker
				if (number_4byte_chunks == 0) {
					// infer ACTUAL number of 4-byte chunks from the length of the color data
					v += 4*(len_bottom+1);
					break;
				}

				// infer the number of bottom colors in next span from chunk length
				len_top = (number_4byte_chunks-1)-len_bottom;

				// now skip the v pointer past the data to the beginning of the next span
				v += v[0]*4;

				bottom_color_end   = v[3]; // aka air start
				bottom_color_start = bottom_color_end-len_top;

				for(z=bottom_color_start;z<bottom_color_end;z++) {
					map_vxl_setcolor(x,y,z,*color++,map);
				}
			}
		}
	}
	for(int k=0;k<map_size_x*map_size_z;k++)
		pthread_rwlock_unlock(&chunk_map_locks[k]);
}
