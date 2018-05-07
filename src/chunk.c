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
#include <pthread.h>
#include <signal.h>

struct chunk chunks[CHUNKS_PER_DIM*CHUNKS_PER_DIM] = {0};

int chunk_geometry_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
int chunk_geometry_changed_lenght = 0;

int chunk_lighting_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
int chunk_lighting_changed_lenght = 0;

int chunk_render_mode = 0;

struct chunk_d {
	int x,y;
};

struct chunk_worker chunk_workers[CHUNK_WORKERS_MAX];

pthread_rwlock_t* chunk_map_locks;
pthread_mutex_t chunk_minimap_lock;

static int chunk_sort(const void* a, const void* b) {
	struct chunk_d* aa = (struct chunk_d*)a;
	struct chunk_d* bb = (struct chunk_d*)b;
	return	distance2D(aa->x+CHUNK_SIZE/2,aa->y+CHUNK_SIZE/2,camera_x,camera_z)
			-distance2D(bb->x+CHUNK_SIZE/2,bb->y+CHUNK_SIZE/2,camera_x,camera_z);
}

void chunk_init() {
	pthread_mutex_init(&chunk_minimap_lock,NULL);
	chunk_map_locks = malloc(map_size_x*map_size_z*sizeof(pthread_rwlock_t));
	CHECK_ALLOCATION_ERROR(chunk_map_locks);
	for(int k=0;k<map_size_x*map_size_z;k++)
		pthread_rwlock_init(&chunk_map_locks[k],NULL);
	for(int k=0;k<CHUNK_WORKERS_MAX;k++) {
		chunk_workers[k].state = CHUNK_WORKERSTATE_IDLE;
		chunk_workers[k].mem_size = 0;
		chunk_workers[k].vertex_data = NULL;
		chunk_workers[k].color_data = NULL;
		pthread_mutex_init(&chunk_workers[k].state_lock,NULL);
		pthread_create(&chunk_workers[k].thread,NULL,chunk_generate,&chunk_workers[k]);
	}
}

void chunk_draw_visible() {
	struct chunk_d chunks_draw[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
	int index = 0;

	//go through all possible chunks and store all in range and view
	for(char y=-9;y<CHUNKS_PER_DIM+9;y++) {
		for(char x=-9;x<CHUNKS_PER_DIM+9;x++) {
			if(((x*CHUNK_SIZE-camera_x)*(x*CHUNK_SIZE-camera_x)+(y*CHUNK_SIZE-camera_z)*(y*CHUNK_SIZE-camera_z))<(settings.render_distance+CHUNK_SIZE)*(settings.render_distance+CHUNK_SIZE)) {
				int tmp_x = x, tmp_y = y;
				if(tmp_x<0)
					tmp_x += CHUNKS_PER_DIM;
				if(tmp_y<0)
					tmp_y += CHUNKS_PER_DIM;
				if(tmp_x>=CHUNKS_PER_DIM)
					tmp_x -= CHUNKS_PER_DIM;
				if(tmp_y>=CHUNKS_PER_DIM)
					tmp_y -= CHUNKS_PER_DIM;

				if(camera_CubeInFrustum(x*CHUNK_SIZE+CHUNK_SIZE/2,0.0F,y*CHUNK_SIZE+CHUNK_SIZE/2,CHUNK_SIZE/2,chunks[tmp_y*CHUNKS_PER_DIM+tmp_x].max_height))
					chunks_draw[index++] = (struct chunk_d){x*CHUNK_SIZE,y*CHUNK_SIZE};
			}
		}
	}

	//sort all chunks to draw those in front first
	qsort(chunks_draw,index,sizeof(struct chunk_d),chunk_sort);

	for(int k=0;k<index;k++)
		chunk_render(chunks_draw[k].x/CHUNK_SIZE,chunks_draw[k].y/CHUNK_SIZE);
}

void chunk_set_render_mode(int r) {
	chunk_render_mode = r;
}

void chunk_rebuild_all() {
	for(int d=0;d<CHUNKS_PER_DIM*CHUNKS_PER_DIM;d++) {
		chunk_geometry_changed[d] = d;
	}
	chunk_geometry_changed_lenght = CHUNKS_PER_DIM*CHUNKS_PER_DIM;
}

void chunk_render(int x, int y) {
	matrix_push();
	matrix_translate((x<0)*-map_size_x+(x>=CHUNKS_PER_DIM)*map_size_x,0.0F,(y<0)*-map_size_z+(y>=CHUNKS_PER_DIM)*map_size_z);
	matrix_upload();

	if(x<0)
		x += CHUNKS_PER_DIM;
	if(y<0)
		y += CHUNKS_PER_DIM;
	if(x>=CHUNKS_PER_DIM)
		x -= CHUNKS_PER_DIM;
	if(y>=CHUNKS_PER_DIM)
		y -= CHUNKS_PER_DIM;

	if(chunks[((y*CHUNKS_PER_DIM)|x)].created) {
		/*if(chunk_render_mode)
			glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);*/
		glx_displaylist_draw(chunks[((y*CHUNKS_PER_DIM)|x)].display_list,chunks[((y*CHUNKS_PER_DIM)|x)].vertex_count);
		/*if(chunk_render_mode)
			glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);*/
	}

	/*glBegin(GL_QUADS);
	glVertex3f(x*CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE+CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE+CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE+CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE+CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE);
	glEnd();*/

	matrix_pop();
}

//return type is important
unsigned long long vertexAO(int side1, int side2, int corner) {
	if(side1!=0xFFFFFFFF || side2!=0xFFFFFFFF || corner!=0xFFFFFFFF) {
		return 0;
	}
	return 1;
}

int chunk_find_edge(float* data, int length, int exclude, float x1, float y1, float z1, float x2, float y2, float z2) {
	for(int k=0;k<length;k+=12) {
		if(k!=exclude) {
			if(((data[k+0]==x1 && data[k+3]==x2) || (data[k+0]==x2 && data[k+3]==x1)) && ((data[k+1]==y1 && data[k+4]==y2) || (data[k+1]==y2 && data[k+4]==y1)) && ((data[k+2]==z1 && data[k+5]==z2) || (data[k+2]==z2 && data[k+5]==z1))) {
				return k;
			}
			if(((data[k+3]==x1 && data[k+6]==x2) || (data[k+3]==x2 && data[k+6]==x1)) && ((data[k+4]==y1 && data[k+7]==y2) || (data[k+4]==y2 && data[k+7]==y1)) && ((data[k+5]==z1 && data[k+8]==z2) || (data[k+5]==z2 && data[k+8]==z1))) {
				return k;
			}
			if(((data[k+6]==x1 && data[k+9]==x2) || (data[k+6]==x2 && data[k+9]==x1)) && ((data[k+7]==y1 && data[k+10]==y2) || (data[k+7]==y2 && data[k+10]==y1)) && ((data[k+8]==z1 && data[k+11]==z2) || (data[k+8]==z2 && data[k+11]==z1))) {
				return k;
			}
			if(((data[k+9]==x1 && data[k+0]==x2) || (data[k+9]==x2 && data[k+0]==x1)) && ((data[k+10]==y1 && data[k+1]==y2) || (data[k+10]==y2 && data[k+1]==y1)) && ((data[k+11]==z1 && data[k+2]==z2) || (data[k+11]==z2 && data[k+2]==z1))) {
				return k;
			}
		}
	}
	return -1;
}

float chunk_face_light(float* data, int k, float lx, float ly, float lz) {
	float x = (data[k+4]-data[k+1])*(data[k+11]-data[k+2])-(data[k+5]-data[k+2])*(data[k+10]-data[k+1]);
	float y = (data[k+5]-data[k+2])*(data[k+9]-data[k+0])-(data[k+3]-data[k+0])*(data[k+11]-data[k+2]);
	float z = (data[k+3]-data[k+0])*(data[k+10]-data[k+1])-(data[k+4]-data[k+1])*(data[k+9]-data[k+0]);
	float len = sqrt(x*x+y*y+z*z);
	return (x/len)*lx+(y/len)*ly+(z/len)*lz;
}

void chunk_draw_shadow_volume(float* data, int max) {
	/*glBegin(GL_QUADS);
	glColor3f(1.0F,0.0F,0.0F);
	float lx = 0.0F;
	float ly = 1.0F;
	float lz = 1.0F;
	for(int k=0;k<max;k+=12) {
		if(chunk_face_light(data,k,lx,ly,lz)>=0.0F) { //visible from light's point of view
			int edge_a = chunk_find_edge(data,max,k,data[k+0],data[k+1],data[k+2],data[k+3],data[k+4],data[k+5]);
			int edge_b = chunk_find_edge(data,max,k,data[k+3],data[k+4],data[k+5],data[k+6],data[k+7],data[k+8]);
			int edge_c = chunk_find_edge(data,max,k,data[k+6],data[k+7],data[k+8],data[k+9],data[k+10],data[k+11]);
			int edge_d = chunk_find_edge(data,max,k,data[k+9],data[k+10],data[k+11],data[k+0],data[k+1],data[k+2]);
			if(edge_a!=-1 && chunk_face_light(data,edge_a,lx,ly,lz)<0.0F) {
				glVertex3f(data[k+0],data[k+1],data[k+2]);
				glVertex3f(data[k+0]-lx*data[k+1]*1.4141F,data[k+1]-ly*data[k+1]*1.4141F,data[k+2]-lz*data[k+1]*1.4141F);
				glVertex3f(data[k+3]-lx*data[k+4]*1.4141F,data[k+4]-ly*data[k+4]*1.4141F,data[k+5]-lz*data[k+4]*1.4141F);
				glVertex3f(data[k+3],data[k+4],data[k+5]);
			}
			if(edge_b!=-1 && chunk_face_light(data,edge_b,lx,ly,lz)<0.0F) {
				glVertex3f(data[k+3],data[k+4],data[k+5]);
				glVertex3f(data[k+3]-lx*data[k+4]*1.4141F,data[k+4]-ly*data[k+4]*1.4141F,data[k+5]-lz*data[k+4]*1.4141F);
				glVertex3f(data[k+6]-lx*data[k+7]*1.4141F,data[k+7]-ly*data[k+7]*1.4141F,data[k+8]-lz*data[k+7]*1.4141F);
				glVertex3f(data[k+6],data[k+7],data[k+8]);
			}
			if(edge_c!=-1 && chunk_face_light(data,edge_c,lx,ly,lz)<0.0F) {
				glVertex3f(data[k+6],data[k+7],data[k+8]);
				glVertex3f(data[k+6]-lx*data[k+7]*1.4141F,data[k+7]-ly*data[k+7]*1.4141F,data[k+8]-lz*data[k+7]*1.4141F);
				glVertex3f(data[k+9]-lx*data[k+10]*1.4141F,data[k+10]-ly*data[k+10]*1.4141F,data[k+11]-lz*data[k+10]*1.4141F);
				glVertex3f(data[k+9],data[k+10],data[k+11]);
			}
			if(edge_d!=-1 && chunk_face_light(data,edge_d,lx,ly,lz)<0.0F) {
				glVertex3f(data[k+9],data[k+10],data[k+11]);
				glVertex3f(data[k+9]-lx*data[k+10]*1.4141F,data[k+10]-ly*data[k+10]*1.4141F,data[k+11]-lz*data[k+10]*1.4141F);
				glVertex3f(data[k+0]-lx*data[k+1]*1.4141F,data[k+1]-ly*data[k+1]*1.4141F,data[k+2]-lz*data[k+1]*1.4141F);
				glVertex3f(data[k+0],data[k+1],data[k+2]);
			}
		}
	}
	glEnd();*/
}

//see this for details: https://github.com/infogulch/pyspades/blob/protocol075/pyspades/vxl_c.cpp#L380
float sunblock(int x, int y, int z) {
    int dec = 18;
    int i = 127;

    while(dec && y<64) {
        if((map_get(x,++y,--z)&0xFFFFFFFF)!=0xFFFFFFFF)
            i -= dec;
        dec -= 2;
    }
    return (float)i/127.0F;
}

void* chunk_generate(void* data) {
	struct chunk_worker* worker = (struct chunk_worker*)data;

	while(1) {
		int state = -1;
		pthread_mutex_lock(&worker->state_lock);
		state = worker->state;
		pthread_mutex_unlock(&worker->state_lock);

		if(state==CHUNK_WORKERSTATE_BUSY) {
			pthread_mutex_lock(&chunk_minimap_lock);
			for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
				for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
					if((x%64)>0 && (z%64)>0) {
						pthread_rwlock_rdlock(&chunk_map_locks[x+z*map_size_x]);
						for(int y=map_size_y-1;y>=0;y--) {
							if(map_colors[x+(y*map_size_z+z)*map_size_x]!=0xFFFFFFFF) {
								map_minimap[(x+z*map_size_x)*4+0] = red(map_colors[x+(y*map_size_z+z)*map_size_x]);
								map_minimap[(x+z*map_size_x)*4+1] = green(map_colors[x+(y*map_size_z+z)*map_size_x]);
								map_minimap[(x+z*map_size_x)*4+2] = blue(map_colors[x+(y*map_size_z+z)*map_size_x]);
								break;
							}
						}
						pthread_rwlock_unlock(&chunk_map_locks[x+z*map_size_x]);
					}
				}
			}
			pthread_mutex_unlock(&chunk_minimap_lock);

			if(settings.greedy_meshing)
				chunk_generate_greedy(worker);
			else
				chunk_generate_naive(worker);

			pthread_mutex_lock(&worker->state_lock);
			worker->state = CHUNK_WORKERSTATE_FINISHED;
			pthread_mutex_unlock(&worker->state_lock);
		}

		usleep(10);
	}
	return NULL;
}

void chunk_generate_greedy(struct chunk_worker* worker) {
	int size = 0;
	int max_height = 0;
	for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
		for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
			for(int y=0;y<map_size_y;y++) {
				int index = x+(y*map_size_z+z)*map_size_x;
				if(map_colors[index]!=0xFFFFFFFF) {
					if(max_height<y) {
						max_height = y;
					}

					//unsigned char shadowed = 0;
					/*for(int a=0;a<map_size_y-y;a++) {
						if(!(shadowed&1)) {
							if(map_get(x,y+a+1,z+a)!=0xFFFFFFFF) {
								shadowed |= 1;
							} else {
								if(map_get(x,y+a+1,z+a+1)!=0xFFFFFFFF || map_get(x,y+a+2,z+a)!=0xFFFFFFFF) {
									shadowed |= 1;
								}
							}
						}
						if(!(shadowed&2)) {
							if(map_get(x,y+a-1,z+a)!=0xFFFFFFFF) {
								shadowed |= 2;
							} else {
								if(map_get(x,y+a-1,z+a+1)!=0xFFFFFFFF || map_get(x,y+a,z+a)!=0xFFFFFFFF) {
									shadowed |= 2;
								}
							}
						}
						if(!(shadowed&4)) {
							if(map_get(x+1,y+a,z+a)!=0xFFFFFFFF) {
								shadowed |= 4;
							} else {
								if(map_get(x+1,y+a,z+a+1)!=0xFFFFFFFF || map_get(x+1,y+a+1,z+a)!=0xFFFFFFFF) {
									shadowed |= 4;
								}
							}
						}
						if(!(shadowed&8)) {
							if(map_get(x-1,y+a,z+a)!=0xFFFFFFFF) {
								shadowed |= 8;
							} else {
								if(map_get(x-1,y+a,z+a+1)!=0xFFFFFFFF || map_get(x-1,y+a+1,z+a)!=0xFFFFFFFF) {
									shadowed |= 8;
								}
							}
						}
						if(!(shadowed&16)) {
							if(map_get(x,y+a,z+a+1)!=0xFFFFFFFF) {
								shadowed |= 16;
							} else {
								if(map_get(x,y+a,z+a+2)!=0xFFFFFFFF || map_get(x,y+a+1,z+a+1)!=0xFFFFFFFF) {
									shadowed |= 16;
								}
							}
						}
						if(!(shadowed&32)) {
							if(map_get(x,y+a,z+a-1)!=0xFFFFFFFF) {
								shadowed |= 32;
							} else {
								if(map_get(x,y+a,z+a)!=0xFFFFFFFF || map_get(x,y+a+1,z+a-1)!=0xFFFFFFFF) {
									shadowed |= 32;
								}
							}
						}
						if(shadowed==0x3F) {
							break;
						}
					}*/

					map_colors[index] &= 0x00FFFFFF;
					//map_colors[index] |= (shadowed<<24);

					if(settings.ambient_occlusion) {
						//positive y
						map_colors[index] |= vertexAO(map_get(x-1,y+1,z),map_get(x,y+1,z+1),map_get(x-1,y+1,z+1))<<32;
						map_colors[index] |= vertexAO(map_get(x+1,y+1,z),map_get(x,y+1,z+1),map_get(x+1,y+1,z+1))<<33;
						map_colors[index] |= vertexAO(map_get(x-1,y+1,z),map_get(x,y+1,z-1),map_get(x-1,y+1,z-1))<<34;
						map_colors[index] |= vertexAO(map_get(x+1,y+1,z),map_get(x,y+1,z-1),map_get(x+1,y+1,z-1))<<35;

						//negative y
						map_colors[index] |= vertexAO(map_get(x-1,y-1,z),map_get(x,y-1,z+1),map_get(x-1,y-1,z+1))<<36;
						map_colors[index] |= vertexAO(map_get(x+1,y-1,z),map_get(x,y-1,z+1),map_get(x+1,y-1,z+1))<<37;
						map_colors[index] |= vertexAO(map_get(x-1,y-1,z),map_get(x,y-1,z-1),map_get(x-1,y-1,z-1))<<38;
						map_colors[index] |= vertexAO(map_get(x+1,y-1,z),map_get(x,y-1,z-1),map_get(x+1,y-1,z-1))<<39;

						//positive x
						map_colors[index] |= vertexAO(map_get(x+1,y,z-1),map_get(x+1,y+1,z),map_get(x+1,y+1,z-1))<<40;
						map_colors[index] |= vertexAO(map_get(x+1,y,z+1),map_get(x+1,y+1,z),map_get(x+1,y+1,z+1))<<41;
						map_colors[index] |= vertexAO(map_get(x+1,y,z-1),map_get(x+1,y-1,z),map_get(x+1,y-1,z-1))<<42;
						map_colors[index] |= vertexAO(map_get(x+1,y,z+1),map_get(x+1,y-1,z),map_get(x+1,y-1,z+1))<<43;

						//negative x
						map_colors[index] |= vertexAO(map_get(x-1,y,z-1),map_get(x-1,y+1,z),map_get(x-1,y+1,z-1))<<44;
						map_colors[index] |= vertexAO(map_get(x-1,y,z+1),map_get(x-1,y+1,z),map_get(x-1,y+1,z+1))<<45;
						map_colors[index] |= vertexAO(map_get(x-1,y,z-1),map_get(x-1,y-1,z),map_get(x-1,y-1,z-1))<<46;
						map_colors[index] |= vertexAO(map_get(x-1,y,z+1),map_get(x-1,y-1,z),map_get(x-1,y-1,z+1))<<47;

						//positive z
						map_colors[index] |= vertexAO(map_get(x-1,y,z+1),map_get(x,y+1,z+1),map_get(x-1,y+1,z+1))<<48;
						map_colors[index] |= vertexAO(map_get(x+1,y,z+1),map_get(x,y+1,z+1),map_get(x+1,y+1,z+1))<<49;
						map_colors[index] |= vertexAO(map_get(x-1,y,z+1),map_get(x,y-1,z+1),map_get(x-1,y-1,z+1))<<50;
						map_colors[index] |= vertexAO(map_get(x+1,y,z+1),map_get(x,y-1,z+1),map_get(x+1,y-1,z+1))<<51;

						//negative z
						map_colors[index] |= vertexAO(map_get(x-1,y,z-1),map_get(x,y+1,z-1),map_get(x-1,y+1,z-1))<<52;
						map_colors[index] |= vertexAO(map_get(x+1,y,z-1),map_get(x,y+1,z-1),map_get(x+1,y+1,z-1))<<53;
						map_colors[index] |= vertexAO(map_get(x-1,y,z-1),map_get(x,y-1,z-1),map_get(x-1,y-1,z-1))<<54;
						map_colors[index] |= vertexAO(map_get(x+1,y,z-1),map_get(x,y-1,z-1),map_get(x+1,y-1,z-1))<<55;
					} else {
						map_colors[index] |= 0xFFFFFFFF00000000;
					}

					if(y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) { size++; }
					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) { size++; }
					if((x==0 && map_colors[map_size_x-1+(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[index-1]==0xFFFFFFFF)) { size++; }
					if((x==map_size_x-1 && map_colors[(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[index+1]==0xFFFFFFFF)) { size++; }
					if((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x]==0xFFFFFFFF)) { size++; }
					if((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x]==0xFFFFFFFF)) { size++; }
				}
			}
		}
	}
	max_height++;

	short* chunk_vertex_data = malloc(size*24);
	CHECK_ALLOCATION_ERROR(chunk_vertex_data)
	unsigned char* chunk_color_data = malloc(size*12);
	CHECK_ALLOCATION_ERROR(chunk_color_data)
	int chunk_vertex_index = 0;
	int chunk_color_index = 0;
	unsigned char checked_voxels[2][CHUNK_SIZE*CHUNK_SIZE];
	unsigned char checked_voxels2[2][CHUNK_SIZE*map_size_y];

	for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
		for(int k=0;k<CHUNK_SIZE*map_size_y;k++) {
			checked_voxels2[0][k] = 0;
			checked_voxels2[1][k] = 0;
		}
		for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
			for(int y=0;y<map_size_y;y++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x]==0xFFFFFFFF)) {
						if(checked_voxels2[0][y+(x-worker->chunk_x)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_x1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_x2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF00000E0FFFFFF)==(col&0xF00000E0FFFFFF) && checked_voxels2[0][y+a+(x-worker->chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xF00000E0FFFFFF)!=(col&0xF00000E0FFFFFF) || checked_voxels2[0][y+a+(x+b-worker->chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x+b]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x+b]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_x1++;
								} else {
									break;
								}
							}

							/*for(unsigned char a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF00000E0FFFFFF)==(col&0xF00000E0FFFFFF) && checked_voxels2[0][y+(x+a-worker->chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xF00000E0FFFFFF)!=(col&0xF00000E0FFFFFF) || checked_voxels2[0][y+b+(x+a-worker->chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+b)*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+b)*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_x2) {
									len_y2++;
								} else {
									break;
								}
							}*/

							unsigned char len_x, len_y;

							if(len_y1*len_x1>len_y2*len_x2) {
								len_y = len_y1;
								len_x = len_x1;
							} else {
								len_y = len_y2;
								len_x = len_x2;
							}

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(x+b-worker->chunk_x)*map_size_y] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&32) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>54)&1)+((col>>53)&1) > ((col>>52)&1)+((col>>55)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>54)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>54)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>54)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>52)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>52)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>52)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>53)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>53)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>53)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>55)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>55)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>55)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>52)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>52)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>52)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>53)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>53)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>53)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>55)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>55)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>55)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.7F*s*(((col>>54)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.7F*s*(((col>>54)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.7F*s*(((col>>54)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							}
						}
					}

					if((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x]==0xFFFFFFFF)) {
						if(checked_voxels2[1][y+(x-worker->chunk_x)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_x1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_x2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF0000D0FFFFFF)==(col&0xF0000D0FFFFFF) && checked_voxels2[1][y+a+(x-worker->chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xF0000D0FFFFFF)!=(col&0xF0000D0FFFFFF) || checked_voxels2[1][y+a+(x+b-worker->chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x+b]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x+b]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_x1++;
								} else {
									break;
								}
							}


							/*for(unsigned char a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF0000D0FFFFFF)==(col&0xF0000D0FFFFFF) && checked_voxels2[1][y+(x+a-worker->chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xF0000D0FFFFFF)!=(col&0xF0000D0FFFFFF) || checked_voxels2[1][y+b+(x+a-worker->chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+b)*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+b)*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_x2) {
									len_y2++;
								} else {
									break;
								}
							}*/

							unsigned char len_x, len_y;

							if(len_y1*len_x1>len_y2*len_x2) {
								len_y = len_y1;
								len_x = len_x1;
							} else {
								len_y = len_y2;
								len_x = len_x2;
							}

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(x+b-worker->chunk_x)*map_size_y] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&16) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>50)&1)+((col>>49)&1) > ((col>>51)&1)+((col>>48)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>50)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>50)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>50)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>51)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>51)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>51)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>49)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>49)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>49)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>48)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>48)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>48)&1)*0.35F+0.65F));


								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y,
								chunk_vertex_data[chunk_vertex_index++] = z+1;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>51)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>51)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>51)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>49)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>49)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>49)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>48)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>48)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>48)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.6F*s*(((col>>50)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.6F*s*(((col>>50)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.6F*s*(((col>>50)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y,
								chunk_vertex_data[chunk_vertex_index++] = z+1;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+1;
							}
						}
					}
				}
			}
		}
	}

	for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
		for(int k=0;k<CHUNK_SIZE*map_size_y;k++) {
			checked_voxels2[0][k] = 0;
			checked_voxels2[1][k] = 0;
		}
		for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
			for(int y=0;y<map_size_y;y++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((x==0 && map_colors[map_size_x-1+(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z)*map_size_x-1]==0xFFFFFFFF)) {
						if(checked_voxels2[0][y+(z-worker->chunk_y)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF000C8FFFFFF)==(col&0xF000C8FFFFFF) && checked_voxels2[0][y+a+(z-worker->chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z)*map_size_x-1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xF000C8FFFFFF)!=(col&0xF000C8FFFFFF) || checked_voxels2[0][y+a+(z+b-worker->chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x-1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_z1++;
								} else {
									break;
								}
							}


							/*for(unsigned char a=0;a<(worker->chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF000C8FFFFFF)==(col&0xF000C8FFFFFF) && checked_voxels2[0][y+(z+a-worker->chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xF000C8FFFFFF)!=(col&0xF000C8FFFFFF) || checked_voxels2[0][y+b+(z+a-worker->chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_z2) {
									len_y2++;
								} else {
									break;
								}
							}*/

							unsigned char len_y, len_z;

							if(len_y1*len_z1>len_y2*len_z2) {
								len_y = len_y1;
								len_z = len_z1;
							} else {
								len_y = len_y2;
								len_z = len_z2;
							}

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(z+b-worker->chunk_y)*map_size_y] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&8) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>46)&1)+((col>>45)&1) > ((col>>47)&1)+((col>>44)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>46)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>46)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>46)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>47)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>47)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>47)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>45)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>45)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>45)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>44)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>44)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>44)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>47)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>47)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>47)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>45)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>45)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>45)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>44)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>44)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>44)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s*(((col>>46)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s*(((col>>46)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s*(((col>>46)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							}
						}
					}
					if((x==map_size_x-1 && map_colors[(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z)*map_size_x+1]==0xFFFFFFFF)) {
						if(checked_voxels2[1][y+(z-worker->chunk_y)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF00C4FFFFFF)==(col&0xF00C4FFFFFF) && checked_voxels2[1][y+a+(z-worker->chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z)*map_size_x+1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xF00C4FFFFFF)!=(col&0xF00C4FFFFFF) || checked_voxels2[1][y+a+(z+b-worker->chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x+1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_z1++;
								} else {
									break;
								}
							}


							/*for(unsigned char a=0;a<(worker->chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF00C4FFFFFF)==(col&0xF00C4FFFFFF) && checked_voxels2[1][y+(z+a-worker->chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xF00C4FFFFFF)!=(col&0xF00C4FFFFFF) || checked_voxels2[1][y+b+(z+a-worker->chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_z2) {
									len_y2++;
								} else {
									break;
								}
							}*/

							unsigned char len_y, len_z;

							if(len_y1*len_z1>len_y2*len_z2) {
								len_y = len_y1;
								len_z = len_z1;
							} else {
								len_y = len_y2;
								len_z = len_z2;
							}

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(z+b-worker->chunk_y)*map_size_y] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&4) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>42)&1)+((col>>41)&1) > ((col>>40)&1)+((col>>43)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>42)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>42)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>42)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>40)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>40)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>40)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>41)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>41)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>41)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>43)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>43)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>43)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y,
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>40)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>40)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>40)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>41)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>41)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>41)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>43)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>43)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>43)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s*(((col>>42)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s*(((col>>42)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s*(((col>>42)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y+len_y,
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+1;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							}
						}
					}
				}
			}
		}
	}

	for(int y=0;y<map_size_y;y++) {
		for(int k=0;k<CHUNK_SIZE*CHUNK_SIZE;k++) {
			checked_voxels[0][k] = 0;
			checked_voxels[1][k] = 0;
		}
		for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
			for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if(y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if(checked_voxels[0][(x-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0) {
							unsigned char len_x1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_x2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xFC1FFFFFF)==(col&0xFC1FFFFFF) && checked_voxels[0][(x+a-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xFC1FFFFFF)!=(col&0xFC1FFFFFF) || checked_voxels[0][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_x1) {
									len_z1++;
								} else {
									break;
								}
							}


							/*for(unsigned char a=0;a<(worker->chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xFC1FFFFFF)==(col&0xFC1FFFFFF) && checked_voxels[0][(x-worker->chunk_x)+(z+a-worker->chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xFC1FFFFFF)!=(col&0xFC1FFFFFF) || checked_voxels[0][(x+b-worker->chunk_x)+(z+a-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+b+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_z2) {
									len_x2++;
								} else {
									break;
								}
							}*/

							unsigned char len_x, len_z;

							if(len_x1*len_z1>len_x2*len_z2) {
								len_x = len_x1;
								len_z = len_z1;
							} else {
								len_x = len_x2;
								len_z = len_z2;
							}

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[0][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&1) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>34)&1)+((col>>33)&1) > ((col>>32)&1)+((col>>35)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>34)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>34)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>34)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>32)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>32)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>32)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>33)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>33)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>33)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>35)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>35)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>35)&1)*0.35F+0.65F));


								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>32)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>32)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>32)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>33)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>33)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>33)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>35)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>35)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>35)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*s*(((col>>34)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*s*(((col>>34)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*s*(((col>>34)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y+1;
								chunk_vertex_data[chunk_vertex_index++] = z;
							}
						}
					}
					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if(checked_voxels[1][(x-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0) {
							unsigned char len_x1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_x2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF0C2FFFFFF)==(col&0xF0C2FFFFFF) && checked_voxels[1][(x+a-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xF0C2FFFFFF)!=(col&0xF0C2FFFFFF) || checked_voxels[1][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_x1) {
									len_z1++;
								} else {
									break;
								}
							}


							/*for(unsigned char a=0;a<(worker->chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF0C2FFFFFF)==(col&0xF0C2FFFFFF) && checked_voxels[1][(x-worker->chunk_x)+(z+a-worker->chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xF0C2FFFFFF)!=(col&0xF0C2FFFFFF) || checked_voxels[1][(x+b-worker->chunk_x)+(z+a-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_z2) {
									len_x2++;
								} else {
									break;
								}
							}*/

							unsigned char len_x, len_z;

							if(len_x1*len_z1>len_x2*len_z2) {
								len_x = len_x1;
								len_z = len_z1;
							} else {
								len_x = len_x2;
								len_z = len_z2;
							}

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[1][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE] = 1;
								}
							}

							float s = 1.0F;

							/*if((map_get(x,y,z)>>24)&2) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}*/

							if(((col>>38)&1)+((col>>37)&1) > ((col>>36)&1)+((col>>39)&1)) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>38)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>38)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>38)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>39)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>39)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>39)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>37)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>37)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>37)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>36)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>36)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>36)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;
							} else {
								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>39)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>39)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>39)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>37)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>37)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>37)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>36)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>36)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>36)&1)*0.35F+0.65F));

								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s*(((col>>38)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s*(((col>>38)&1)*0.35F+0.65F));
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s*(((col>>38)&1)*0.35F+0.65F));

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;

								chunk_vertex_data[chunk_vertex_index++] = x+len_x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z+len_z;

								chunk_vertex_data[chunk_vertex_index++] = x;
								chunk_vertex_data[chunk_vertex_index++] = y;
								chunk_vertex_data[chunk_vertex_index++] = z;
							}
						}
					}
				}
			}
		}
	}

	worker->vertex_data = chunk_vertex_data;
	worker->color_data = chunk_color_data;
	worker->data_size = chunk_vertex_index/3;
	worker->max_height = max_height;
}

void chunk_generate_naive(struct chunk_worker* worker) {
	int size = 0;
	int max_height = 0;
	int chunk_vertex_index = 0;
	int chunk_color_index = 0;

	for(int z=worker->chunk_y;z<worker->chunk_y+CHUNK_SIZE;z++) {
		for(int x=worker->chunk_x;x<worker->chunk_x+CHUNK_SIZE;x++) {
			pthread_rwlock_rdlock(&chunk_map_locks[x+z*map_size_x]);
			for(int y=0;y<map_size_y;y++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					if(max_height<y) {
						max_height = y;
					}
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					float shade = sunblock(x,y,z);
					r *= shade;
					g *= shade;
					b *= shade;
					if(z==0) {
						pthread_rwlock_rdlock(&chunk_map_locks[x+(map_size_z-1)*map_size_x]);
					}
					if(z>0) {
						pthread_rwlock_rdlock(&chunk_map_locks[x+(z-1)*map_size_x]);
					}
					if((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x]==0xFFFFFFFF)) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*0.7F*(((col>>54)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.7F*(((col>>54)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.7F*(((col>>54)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.7F*(((col>>52)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.7F*(((col>>52)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.7F*(((col>>52)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.7F*(((col>>53)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.7F*(((col>>53)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.7F*(((col>>53)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.7F*(((col>>55)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.7F*(((col>>55)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.7F*(((col>>55)&1)*0.35F+0.65F));

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;
					}
					if(z==0) {
						pthread_rwlock_unlock(&chunk_map_locks[x+(map_size_z-1)*map_size_x]);
					}
					if(z>0) {
						pthread_rwlock_unlock(&chunk_map_locks[x+(z-1)*map_size_x]);
					}

					if(z==map_size_z-1) {
						pthread_rwlock_rdlock(&chunk_map_locks[x]);
					}
					if(z<map_size_z-1) {
						pthread_rwlock_rdlock(&chunk_map_locks[x+(z+1)*map_size_x]);
					}
					if((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x]==0xFFFFFFFF)) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*0.6F*(((col>>50)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.6F*(((col>>50)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.6F*(((col>>50)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.6F*(((col>>51)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.6F*(((col>>51)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.6F*(((col>>51)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.6F*(((col>>49)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.6F*(((col>>49)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.6F*(((col>>49)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.6F*(((col>>48)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.6F*(((col>>48)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.6F*(((col>>48)&1)*0.35F+0.65F));


						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1,
						worker->vertex_data[chunk_vertex_index++] = z+1;
					}
					if(z==map_size_z-1) {
						pthread_rwlock_unlock(&chunk_map_locks[x]);
					}
					if(z<map_size_z-1) {
						pthread_rwlock_unlock(&chunk_map_locks[x+(z+1)*map_size_x]);
					}

					if(x==0) {
						pthread_rwlock_rdlock(&chunk_map_locks[(map_size_x-1)+z*map_size_x]);
					}
					if(x>0) {
						pthread_rwlock_rdlock(&chunk_map_locks[(x-1)+z*map_size_x]);
					}
					if((x==0 && map_colors[map_size_x-1+(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z)*map_size_x-1]==0xFFFFFFFF)) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*0.9F*(((col>>46)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.9F*(((col>>46)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.9F*(((col>>46)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.9F*(((col>>47)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.9F*(((col>>47)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.9F*(((col>>47)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.9F*(((col>>45)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.9F*(((col>>45)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.9F*(((col>>45)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.9F*(((col>>44)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.9F*(((col>>44)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.9F*(((col>>44)&1)*0.35F+0.65F));

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;
					}
					if(x==0) {
						pthread_rwlock_unlock(&chunk_map_locks[(map_size_x-1)+z*map_size_x]);
					}
					if(x>0) {
						pthread_rwlock_unlock(&chunk_map_locks[(x-1)+z*map_size_x]);
					}

					if(x==map_size_x-1) {
						pthread_rwlock_rdlock(&chunk_map_locks[z*map_size_x]);
					}
					if(x<map_size_x-1) {
						pthread_rwlock_rdlock(&chunk_map_locks[(x+1)+z*map_size_x]);
					}
					if((x==map_size_x-1 && map_colors[(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z)*map_size_x+1]==0xFFFFFFFF)) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*0.8F*(((col>>42)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.8F*(((col>>42)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.8F*(((col>>42)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.8F*(((col>>40)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.8F*(((col>>40)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.8F*(((col>>40)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.8F*(((col>>41)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.8F*(((col>>41)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.8F*(((col>>41)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.8F*(((col>>43)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.8F*(((col>>43)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.8F*(((col>>43)&1)*0.35F+0.65F));

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1,
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;
					}
					if(x==map_size_x-1) {
						pthread_rwlock_unlock(&chunk_map_locks[z*map_size_x]);
					}
					if(x<map_size_x-1) {
						pthread_rwlock_unlock(&chunk_map_locks[(x+1)+z*map_size_x]);
					}

					if(y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*(((col>>34)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*(((col>>34)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*(((col>>34)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*(((col>>32)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*(((col>>32)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*(((col>>32)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*(((col>>33)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*(((col>>33)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*(((col>>33)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*(((col>>35)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*(((col>>35)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*(((col>>35)&1)*0.35F+0.65F));


						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;
					}

					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*24);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*12);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;
						worker->color_data[chunk_color_index++] = (int)(r*0.5F*(((col>>38)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.5F*(((col>>38)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.5F*(((col>>38)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.5F*(((col>>39)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.5F*(((col>>39)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.5F*(((col>>39)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.5F*(((col>>37)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.5F*(((col>>37)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.5F*(((col>>37)&1)*0.35F+0.65F));

						worker->color_data[chunk_color_index++] = (int)(r*0.5F*(((col>>36)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(g*0.5F*(((col>>36)&1)*0.35F+0.65F));
						worker->color_data[chunk_color_index++] = (int)(b*0.5F*(((col>>36)&1)*0.35F+0.65F));

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;
					}
				}
			}
			pthread_rwlock_unlock(&chunk_map_locks[x+z*map_size_x]);
		}
	}

	worker->data_size = chunk_vertex_index/3;
	worker->max_height = max_height+1;
}

void chunk_update_all() {
	for(int j=0;j<CHUNK_WORKERS_MAX;j++) {
		pthread_mutex_lock(&chunk_workers[j].state_lock);
		if(chunk_workers[j].state==CHUNK_WORKERSTATE_FINISHED) {
			//float s3 = window_time();
			chunk_workers[j].state = CHUNK_WORKERSTATE_IDLE;
			chunks[chunk_workers[j].chunk_id].max_height = chunk_workers[j].max_height;
			chunks[chunk_workers[j].chunk_id].vertex_count = chunk_workers[j].data_size;

			glx_displaylist_update(chunks[chunk_workers[j].chunk_id].display_list,chunk_workers[j].data_size,chunk_workers[j].color_data,chunk_workers[j].vertex_data);

			//printf("(s3) %i\n",(int)((window_time()-s3)*1000.0F));
			//s3 = window_time();

			int tex_data[CHUNK_SIZE*CHUNK_SIZE];
			for(int y=0;y<CHUNK_SIZE;y++) {
				for(int x=0;x<CHUNK_SIZE;x++) {
					tex_data[x+y*CHUNK_SIZE] = ((int*)map_minimap)[x+chunk_workers[j].chunk_x+(y+chunk_workers[j].chunk_y)*map_size_x];
				}
			}

			glBindTexture(GL_TEXTURE_2D,texture_minimap.texture_id);
			pthread_mutex_lock(&chunk_minimap_lock);
			glTexSubImage2D(GL_TEXTURE_2D,0,chunk_workers[j].chunk_x,chunk_workers[j].chunk_y,CHUNK_SIZE,CHUNK_SIZE,GL_RGBA,GL_UNSIGNED_BYTE,tex_data);
			pthread_mutex_unlock(&chunk_minimap_lock);
			glBindTexture(GL_TEXTURE_2D,0);

			//free(chunk_workers[j].vertex_data);
			//free(chunk_workers[j].color_data);
			//printf("(s4) %i\n",(int)((window_time()-s3)*1000.0F));
		}
		if(chunk_workers[j].state==CHUNK_WORKERSTATE_IDLE && chunk_geometry_changed_lenght>0) {
			//float s1 = window_time();
			float closest_dist = 1e10;
			int closest_index = 0;
			for(int k=0;k<chunk_geometry_changed_lenght;k++) {
				int chunk_x = (chunk_geometry_changed[k]%CHUNKS_PER_DIM)*CHUNK_SIZE;
				int chunk_y = (chunk_geometry_changed[k]/CHUNKS_PER_DIM)*CHUNK_SIZE;
				float l = (chunk_x-camera_x)*(chunk_x-camera_x)+(chunk_y-camera_z)*(chunk_y-camera_z);
				if(l<closest_dist) {
					closest_dist = l;
					closest_index = k;
				}
			}
			//printf("UPDATE GEOMETRY: %i %i\n",chunk_geometry_changed[closest_index]%CHUNKS_PER_DIM,chunk_geometry_changed[closest_index]/CHUNKS_PER_DIM);
			int chunk_x = (chunk_geometry_changed[closest_index]%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_geometry_changed[closest_index]/CHUNKS_PER_DIM)*CHUNK_SIZE;
			if(!chunks[chunk_geometry_changed[closest_index]].created) {
				chunks[chunk_geometry_changed[closest_index]].display_list = glx_displaylist_create();
			}
			chunk_workers[j].chunk_id = chunk_geometry_changed[closest_index];
			chunk_workers[j].chunk_x = chunk_x;
			chunk_workers[j].chunk_y = chunk_y;
			chunk_workers[j].state = CHUNK_WORKERSTATE_BUSY;
			//float s2 = window_time();
			//pthread_create(&chunk_workers[j].thread,NULL,chunk_generate,&chunk_workers[j]);
			//printf("(s2) thread creation time: %i\n",(int)((window_time()-s2)*1000.0F));
			chunks[chunk_geometry_changed[closest_index]].last_update = window_time();
			chunks[chunk_geometry_changed[closest_index]].created = 1;

			for(int i=closest_index;i<chunk_geometry_changed_lenght-1;i++) {
				chunk_geometry_changed[i] = chunk_geometry_changed[i+1];
			}
			chunk_geometry_changed_lenght--;
			//printf("(s1) %i\n",(int)((window_time()-s1)*1000.0F));
		}
		pthread_mutex_unlock(&chunk_workers[j].state_lock);
	}
}

void chunk_block_update(int x, int y, int z) {
	int d = (z/CHUNK_SIZE)*CHUNKS_PER_DIM+(x/CHUNK_SIZE);
	for(int k=0;k<chunk_geometry_changed_lenght;k++) {
		if(chunk_geometry_changed[k]==d) {
			return;
		}
	}
	chunk_geometry_changed[chunk_geometry_changed_lenght++] = d;
}
