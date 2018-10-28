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
	for(int y=-9;y<CHUNKS_PER_DIM+9;y++) {
		for(int x=-9;x<CHUNKS_PER_DIM+9;x++) {
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

	//glPolygonMode(GL_FRONT,GL_LINE);

	if(chunks[((y*CHUNKS_PER_DIM)|x)].created)
		glx_displaylist_draw(&chunks[((y*CHUNKS_PER_DIM)|x)].display_list,GLX_DISPLAYLIST_NORMAL);

	//glPolygonMode(GL_FRONT,GL_FILL);

	matrix_pop();
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
					if(max_height<y)
						max_height = y;

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
				unsigned int col = map_get(x,y,z);
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((z==0 && map_isair(x,y,map_size_z-1)) || (z>0 && map_isair(x,y,z-1))) {
						if(checked_voxels2[0][y+(x-worker->chunk_x)*map_size_y]==0) {
							int len_y = 0;
							int len_x = 1;

							for(int a=0;a<map_size_y-y;a++) {
								if(map_get(x,y+a,z)==col && checked_voxels2[0][y+a+(x-worker->chunk_x)*map_size_y]==0 && ((z==0 && map_isair(x,y+a,map_size_z-1)) || (z>0 && map_isair(x,y+a,z-1))))
									len_y++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								int a;
								for(a=0;a<len_y;a++) {
									if(map_get(x+b,y+a,z)!=col || checked_voxels2[0][y+a+(x+b-worker->chunk_x)*map_size_y]!=0 || !((z==0 && map_isair(x+b,y+a,map_size_z-1)) || (z>0 && map_isair(x+b,y+a,z-1))))
										break;
								}
								if(a==len_y)
									len_x++;
								else
									break;
							}

							for(int b=0;b<len_x;b++)
								for(int a=0;a<len_y;a++)
									checked_voxels2[0][y+a+(x+b-worker->chunk_x)*map_size_y] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.7F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.7F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.7F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.7F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.7F);

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
						}
					}

					if((z==map_size_z-1 && map_isair(x,y,0)) || (z<map_size_z-1 && map_isair(x,y,z+1))) {
						if(checked_voxels2[1][y+(x-worker->chunk_x)*map_size_y]==0) {
							int len_y = 0;
							int len_x = 1;

							for(int a=0;a<map_size_y-y;a++) {
								if(map_get(x,y+a,z)==col && checked_voxels2[1][y+a+(x-worker->chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_isair(x,y+a,0)) || (z<map_size_z-1 && map_isair(x,y+a,z+1))))
									len_y++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_x+CHUNK_SIZE-x);b++) {
								int a;
								for(a=0;a<len_y;a++) {
									if(map_get(x+b,a+a,z)!=col || checked_voxels2[1][y+a+(x+b-worker->chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_isair(x+b,y+a,0)) || (z<map_size_z-1 && map_isair(x+b,y+a,z+1))))
										break;
								}
								if(a==len_y)
									len_x++;
								else
									break;
							}

							for(int b=0;b<len_x;b++)
								for(int a=0;a<len_y;a++)
									checked_voxels2[1][y+a+(x+b-worker->chunk_x)*map_size_y] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.6F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.6F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.6F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.6F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.6F);


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
							chunk_vertex_data[chunk_vertex_index++] = y+len_y;
							chunk_vertex_data[chunk_vertex_index++] = z+1;
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
				unsigned int col = map_get(x,y,z);
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((x==0 && map_isair(map_size_x-1,y,z)) || (x>0 && map_isair(x-1,y,z))) {
						if(checked_voxels2[0][y+(z-worker->chunk_y)*map_size_y]==0) {
							int len_y = 0;
							int len_z = 1;

							for(int a=0;a<map_size_y-y;a++) {
								if(map_get(x,y+a,z)==col && checked_voxels2[0][y+a+(z-worker->chunk_y)*map_size_y]==0 && ((x==0 && map_isair(map_size_x-1,y+a,z)) || (x>0 && map_isair(x-1,y+a,z))))
									len_y++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								int a;
								for(a=0;a<len_y;a++) {
									if(map_get(x,y+a,z+b)!=col || checked_voxels2[0][y+a+(z+b-worker->chunk_y)*map_size_y]!=0 || !((x==0 && map_isair(map_size_x-1,y+a,z+b)) || (x>0 && map_isair(x-1,y+a,z+b))))
										break;
								}
								if(a==len_y)
									len_z++;
								else
									break;
							}


							for(int b=0;b<len_z;b++)
								for(int a=0;a<len_y;a++)
									checked_voxels2[0][y+a+(z+b-worker->chunk_y)*map_size_y] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.9F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.9F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.9F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.9F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.9F);

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
						}
					}
					if((x==map_size_x-1 && map_isair(0,y,z)) || (x<map_size_x-1 && map_isair(x+1,y,z))) {
						if(checked_voxels2[1][y+(z-worker->chunk_y)*map_size_y]==0) {
							int len_y = 0;
							int len_z = 1;

							for(int a=0;a<map_size_y-y;a++) {
								if(map_get(x,y+a,z)==col && checked_voxels2[1][y+a+(z-worker->chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_isair(0,y+a,z)) || (x<map_size_x-1 && map_isair(x+1,y+a,z))))
									len_y++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								int a;
								for(a=0;a<len_y;a++) {
									if(map_get(x,y+a,z+b)!=col || checked_voxels2[1][y+a+(z+b-worker->chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_isair(0,y+a,z+b)) || (x<map_size_x-1 && map_isair(x+1,y+a,z+b))))
										break;
								}
								if(a==len_y)
									len_z++;
								else
									break;
							}

							for(unsigned char b=0;b<len_z;b++)
								for(unsigned char a=0;a<len_y;a++)
									checked_voxels2[1][y+a+(z+b-worker->chunk_y)*map_size_y] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.8F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.8F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.8F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.8F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.8F);

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
				unsigned int col = map_get(x,y,z);
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if(y==map_size_y-1 || map_isair(x,y+1,z)) {
						if(checked_voxels[0][(x-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0) {
							int len_x = 0;
							int len_z = 1;

							for(int a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if(map_get(x+a,y,z)==col && checked_voxels[0][(x+a-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_isair(x+a,y+1,z)))
									len_x++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								int a;
								for(a=0;a<len_x;a++) {
									if(map_get(x+a,y,z+b)!=col || checked_voxels[0][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_isair(x+a,y+1,z+b)))
										break;
								}
								if(a==len_x)
									len_z++;
								else
									break;
							}

							for(int b=0;b<len_z;b++)
								for(int a=0;a<len_x;a++)
									checked_voxels[0][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r);
							chunk_color_data[chunk_color_index++] = (int)(g);
							chunk_color_data[chunk_color_index++] = (int)(b);

							chunk_color_data[chunk_color_index++] = (int)(r);
							chunk_color_data[chunk_color_index++] = (int)(g);
							chunk_color_data[chunk_color_index++] = (int)(b);

							chunk_color_data[chunk_color_index++] = (int)(r);
							chunk_color_data[chunk_color_index++] = (int)(g);
							chunk_color_data[chunk_color_index++] = (int)(b);

							chunk_color_data[chunk_color_index++] = (int)(r);
							chunk_color_data[chunk_color_index++] = (int)(g);
							chunk_color_data[chunk_color_index++] = (int)(b);


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
						}
					}
					if(y>0 && map_isair(x,y-1,z)) {
						if(checked_voxels[1][(x-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0) {
							int len_x = 0;
							int len_z = 1;

							for(int a=0;a<(worker->chunk_x+CHUNK_SIZE-x);a++) {
								if(map_get(x+a,y,z)==col && checked_voxels[1][(x+a-worker->chunk_x)+(z-worker->chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_isair(x+a,y-1,z)))
									len_x++;
								else
									break;
							}

							for(int b=1;b<(worker->chunk_y+CHUNK_SIZE-z);b++) {
								int a;
								for(a=0;a<len_x;a++) {
									if(map_get(x+a,y,z+b)!=col || checked_voxels[1][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_isair(x+a,y-1,z+b)))
										break;
								}
								if(a==len_x)
									len_z++;
								else
									break;
							}

							for(int b=0;b<len_z;b++)
								for(int a=0;a<len_x;a++)
									checked_voxels[1][(x+a-worker->chunk_x)+(z+b-worker->chunk_y)*CHUNK_SIZE] = 1;

							chunk_color_data[chunk_color_index++] = (int)(r*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.5F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.5F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.5F);

							chunk_color_data[chunk_color_index++] = (int)(r*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(g*0.5F);
							chunk_color_data[chunk_color_index++] = (int)(b*0.5F);

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

//+X = 0.75
//-X = 0.75
//+Y = 1.0
//-Y = 0.5
//+Z = 0.625
//-Z = 0.875

#ifdef OPENGL_ES
	#define VERTEX_ADVANCE	(3*6*sizeof(short))
	#define COLOR_ADVANCE	(4*6*sizeof(unsigned char))
#else
	#define VERTEX_ADVANCE	(3*4*sizeof(short))
	#define COLOR_ADVANCE	(3*4*sizeof(unsigned char))
#endif

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
					float shade = map_sunblock(x,y,z);
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
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r*0.875F);
							worker->color_data[chunk_color_index++] = (int)(g*0.875F);
							worker->color_data[chunk_color_index++] = (int)(b*0.875F);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;
						#endif

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
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r*0.625F);
							worker->color_data[chunk_color_index++] = (int)(g*0.625F);
							worker->color_data[chunk_color_index++] = (int)(b*0.625F);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;
						#endif

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
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r*0.75F);
							worker->color_data[chunk_color_index++] = (int)(g*0.75F);
							worker->color_data[chunk_color_index++] = (int)(b*0.75F);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;
						#endif

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
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r*0.75F);
							worker->color_data[chunk_color_index++] = (int)(g*0.75F);
							worker->color_data[chunk_color_index++] = (int)(b*0.75F);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1,
						worker->vertex_data[chunk_vertex_index++] = z+1;
						#endif

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
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r);
							worker->color_data[chunk_color_index++] = (int)(g);
							worker->color_data[chunk_color_index++] = (int)(b);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z+1;
						#endif

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y+1;
						worker->vertex_data[chunk_vertex_index++] = z;
					}

					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if((size+1)>worker->mem_size) {
							worker->mem_size += 512; //allow for 512 more quads
							worker->vertex_data = realloc(worker->vertex_data,worker->mem_size*VERTEX_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->vertex_data)
							worker->color_data = realloc(worker->color_data,worker->mem_size*COLOR_ADVANCE);
							CHECK_ALLOCATION_ERROR(worker->color_data)
						}
						size++;

						#ifdef OPENGL_ES
						for(int l=0;l<6;l++) {
						#else
						for(int l=0;l<4;l++) {
						#endif
							worker->color_data[chunk_color_index++] = (int)(r*0.5F);
							worker->color_data[chunk_color_index++] = (int)(g*0.5F);
							worker->color_data[chunk_color_index++] = (int)(b*0.5F);
						#ifdef OPENGL_ES
							worker->color_data[chunk_color_index++] = 255;
						#endif
						}

						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;

						#ifdef OPENGL_ES
						worker->vertex_data[chunk_vertex_index++] = x;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z;

						worker->vertex_data[chunk_vertex_index++] = x+1;
						worker->vertex_data[chunk_vertex_index++] = y;
						worker->vertex_data[chunk_vertex_index++] = z+1;
						#endif

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
			chunk_workers[j].state = CHUNK_WORKERSTATE_IDLE;
			chunks[chunk_workers[j].chunk_id].max_height = chunk_workers[j].max_height;
			chunks[chunk_workers[j].chunk_id].vertex_count = chunk_workers[j].data_size;

			glx_displaylist_update(&chunks[chunk_workers[j].chunk_id].display_list,chunk_workers[j].data_size,GLX_DISPLAYLIST_NORMAL,chunk_workers[j].color_data,chunk_workers[j].vertex_data,NULL);

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
		}
		if(chunk_workers[j].state==CHUNK_WORKERSTATE_IDLE && chunk_geometry_changed_lenght>0) {
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
				 glx_displaylist_create(&chunks[chunk_geometry_changed[closest_index]].display_list);
			}
			chunk_workers[j].chunk_id = chunk_geometry_changed[closest_index];
			chunk_workers[j].chunk_x = chunk_x;
			chunk_workers[j].chunk_y = chunk_y;
			chunk_workers[j].state = CHUNK_WORKERSTATE_BUSY;
			chunks[chunk_geometry_changed[closest_index]].last_update = window_time();
			chunks[chunk_geometry_changed[closest_index]].created = 1;
			for(int i=closest_index;i<chunk_geometry_changed_lenght-1;i++) {
				chunk_geometry_changed[i] = chunk_geometry_changed[i+1];
			}
			chunk_geometry_changed_lenght--;
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
