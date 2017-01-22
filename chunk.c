int chunks_to_draw_x[(CHUNKS_PER_DIM+18)*(CHUNKS_PER_DIM+18)] = {0};
int chunks_to_draw_y[(CHUNKS_PER_DIM+18)*(CHUNKS_PER_DIM+18)] = {0};

float chunk_draw_visible(boolean shadowed) {
	int index = 0;
	int b = 0;
	
	//go through all possible chunks and store all in range and view
	for(char y=-9;y<CHUNKS_PER_DIM+9;y++) {
		for(char x=-9;x<CHUNKS_PER_DIM+9;x++) {
			if(((x*CHUNK_SIZE-camera_x)*(x*CHUNK_SIZE-camera_x)+(y*CHUNK_SIZE-camera_z)*(y*CHUNK_SIZE-camera_z))<(settings.render_distance+CHUNK_SIZE*2.0F)*(settings.render_distance+CHUNK_SIZE*2.0F)) {
				int tmp_x = x, tmp_y = y;
				if(tmp_x<0) {
					tmp_x += CHUNKS_PER_DIM;
				}
				if(tmp_y<0) {
					tmp_y += CHUNKS_PER_DIM;
				}
				if(tmp_x>=CHUNKS_PER_DIM) {
					tmp_x -= CHUNKS_PER_DIM;
				}
				if(tmp_y>=CHUNKS_PER_DIM) {
					tmp_y -= CHUNKS_PER_DIM;
				}
				if(camera_CubeInFrustum(x*CHUNK_SIZE+CHUNK_SIZE/2,0.0F,y*CHUNK_SIZE+CHUNK_SIZE/2,CHUNK_SIZE/2,chunk_max_height[tmp_y*CHUNKS_PER_DIM+tmp_x])) {
					chunks_to_draw_x[index] = x*CHUNK_SIZE;
					chunks_to_draw_y[index++] = y*CHUNK_SIZE;
				}
				b++;
			}
		}
	}
	
	//sort all chunks to draw those in front first
	while(1) {
		unsigned char found = 0;
		for(int k=0;k<index-1;k++) {
			if(((chunks_to_draw_x[k]-camera_x)*(chunks_to_draw_x[k]-camera_x)+(chunks_to_draw_y[k]-camera_z)*(chunks_to_draw_y[k]-camera_z))>((chunks_to_draw_x[k+1]-camera_x)*(chunks_to_draw_x[k+1]-camera_x)+(chunks_to_draw_y[k+1]-camera_z)*(chunks_to_draw_y[k+1]-camera_z))) {
				int tmp = chunks_to_draw_x[k];
				chunks_to_draw_x[k] = chunks_to_draw_x[k+1];
				chunks_to_draw_x[k+1] = tmp;

				tmp = chunks_to_draw_y[k];
				chunks_to_draw_y[k] = chunks_to_draw_y[k+1];
				chunks_to_draw_y[k+1] = tmp;
				found = 1;
			}
		}
		if(!found) {
			break;
		}
	}
	
	unsigned int d, chunk_x, chunk_y;
	
	for(int k=0;k<index;k++) {
		chunk_x = chunk_map_coord(chunks_to_draw_x[k]/CHUNK_SIZE);
		chunk_y = chunk_map_coord(chunks_to_draw_y[k]/CHUNK_SIZE);
		d = chunk_y*CHUNKS_PER_DIM+chunk_x;
			
		if(!glIsList(chunk_display_lists[d])) {
			chunk_display_lists[d] = glGenLists(1);
			chunk_display_lists_shadowed[d] = glGenLists(1);
			chunk_max_height[d] = chunk_generate(chunk_display_lists[d],chunk_display_lists_shadowed[d],chunk_x*CHUNK_SIZE,chunk_y*CHUNK_SIZE);
			chunk_last_update[d] = timems();
		}
	}
	
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	for(int k=0;k<index;k++) {
		chunk_render(chunks_to_draw_x[k]/CHUNK_SIZE,chunks_to_draw_y[k]/CHUNK_SIZE,shadowed);
	}
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
	if(b==0) {
		return 0;
	}
	return ((float)index)/((float)b);
}

void chunk_set_render_mode(boolean r) {
	chunk_render_mode = r;
}

void chunk_rebuild_all() {
	chunk_geometry_rebuild = 1;
	chunk_geometry_rebuild_state = 0;
}

int chunk_map_coord(int x) {
	if(x<0) {
		x += CHUNKS_PER_DIM;
	}
	if(x>=CHUNKS_PER_DIM) {
		x -= CHUNKS_PER_DIM;
	}
	return x;
}

void chunk_render(int x, int y, boolean shadowed) {
	glPushMatrix();
	glTranslatef((x<0)*-map_size_x+(x>=CHUNKS_PER_DIM)*map_size_x,0.0F,(y<0)*-map_size_z+(y>=CHUNKS_PER_DIM)*map_size_z);
	x = chunk_map_coord(x);
	y = chunk_map_coord(y);

	if(chunk_render_mode) {
		glLineWidth(4.0F);
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}
	if(shadowed) {
		glCallList(chunk_display_lists_shadowed[((y*CHUNKS_PER_DIM)|x)]);
	} else {
		glCallList(chunk_display_lists[((y*CHUNKS_PER_DIM)|x)]);
	}
	if(chunk_render_mode) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}
	
	glPopMatrix();
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
	glBegin(GL_QUADS);
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
	glEnd();
}

int chunk_generate(int displaylist, int displaylist_shadowed, int chunk_x, int chunk_y) {
	float shadow_mask = 0.75F;
	
	int size = 0;
	int max_height = 0;
	for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
		for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
			for(int y=0;y<map_size_y;y++) {
				int index = x+(y*map_size_z+z)*map_size_x;
				if(map_colors[index]!=0xFFFFFFFF) {
					if(max_height<y) {
						max_height = y;
					}
					
					unsigned char shadowed = 0;
					for(int a=0;a<map_size_y-y;a++) {
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
					}
					
					map_colors[index] &= 0x00FFFFFF;
					map_colors[index] |= (shadowed<<24);
					
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
	unsigned char* chunk_color_data = malloc(size*12);
	unsigned char* chunk_shadow_data = malloc(size);
	int chunk_vertex_index = 0;
	int chunk_color_index = 0;
	int chunk_shadow_index = 0;
	unsigned char checked_voxels[2][CHUNK_SIZE*CHUNK_SIZE];
	unsigned char checked_voxels2[2][CHUNK_SIZE*map_size_y];

	for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
		for(int k=0;k<CHUNK_SIZE*map_size_y;k++) {
			checked_voxels2[0][k] = 0;
			checked_voxels2[1][k] = 0;
		}
		for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
			for(int y=0;y<map_size_y;y++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x]==0xFFFFFFFF)) {
						if(checked_voxels2[0][y+(x-chunk_x)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_x1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_x2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF00000E0FFFFFF)==(col&0xF00000E0FFFFFF) && checked_voxels2[0][y+a+(x-chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xF00000E0FFFFFF)!=(col&0xF00000E0FFFFFF) || checked_voxels2[0][y+a+(x+b-chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x+b]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x+b]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char a=0;a<(chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF00000E0FFFFFF)==(col&0xF00000E0FFFFFF) && checked_voxels2[0][y+(x+a-chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xF00000E0FFFFFF)!=(col&0xF00000E0FFFFFF) || checked_voxels2[0][y+b+(x+a-chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+b)*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+b)*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_x2) {
									len_y2++;
								} else {
									break;
								}
							}

							unsigned char len_x, len_y;

							if(len_y1*len_x1>len_y2*len_x2) {
								len_y = len_y1;
								len_x = len_x1;
							} else {
								len_y = len_y2;
								len_x = len_x2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_x = len_y = 1;
							#endif

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(x+b-chunk_x)*map_size_y] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&32) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}
							
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
						if(checked_voxels2[1][y+(x-chunk_x)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_x1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_x2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF0000D0FFFFFF)==(col&0xF0000D0FFFFFF) && checked_voxels2[1][y+a+(x-chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xF0000D0FFFFFF)!=(col&0xF0000D0FFFFFF) || checked_voxels2[1][y+a+(x+b-chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x+b]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x+b]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_x1++;
								} else {
									break;
								}
							}


							for(unsigned char a=0;a<(chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF0000D0FFFFFF)==(col&0xF0000D0FFFFFF) && checked_voxels2[1][y+(x+a-chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xF0000D0FFFFFF)!=(col&0xF0000D0FFFFFF) || checked_voxels2[1][y+b+(x+a-chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+b)*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+b)*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_x2) {
									len_y2++;
								} else {
									break;
								}
							}

							unsigned char len_x, len_y;

							if(len_y1*len_x1>len_y2*len_x2) {
								len_y = len_y1;
								len_x = len_x1;
							} else {
								len_y = len_y2;
								len_x = len_x2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_x = len_y = 1;
							#endif

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(x+b-chunk_x)*map_size_y] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&16) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}

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

	for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
		for(int k=0;k<CHUNK_SIZE*map_size_y;k++) {
			checked_voxels2[0][k] = 0;
			checked_voxels2[1][k] = 0;
		}
		for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
			for(int y=0;y<map_size_y;y++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if((x==0 && map_colors[map_size_x-1+(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z)*map_size_x-1]==0xFFFFFFFF)) {
						if(checked_voxels2[0][y+(z-chunk_y)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF000C8FFFFFF)==(col&0xF000C8FFFFFF) && checked_voxels2[0][y+a+(z-chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z)*map_size_x-1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xF000C8FFFFFF)!=(col&0xF000C8FFFFFF) || checked_voxels2[0][y+a+(z+b-chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x-1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_z1++;
								} else {
									break;
								}
							}


							for(unsigned char a=0;a<(chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF000C8FFFFFF)==(col&0xF000C8FFFFFF) && checked_voxels2[0][y+(z+a-chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xF000C8FFFFFF)!=(col&0xF000C8FFFFFF) || checked_voxels2[0][y+b+(z+a-chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_z2) {
									len_y2++;
								} else {
									break;
								}
							}

							unsigned char len_y, len_z;

							if(len_y1*len_z1>len_y2*len_z2) {
								len_y = len_y1;
								len_z = len_z1;
							} else {
								len_y = len_y2;
								len_z = len_z2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_y = len_z = 1;
							#endif

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(z+b-chunk_y)*map_size_y] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&8) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}
							
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
						if(checked_voxels2[1][y+(z-chunk_y)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xF00C4FFFFFF)==(col&0xF00C4FFFFFF) && checked_voxels2[1][y+a+(z-chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z)*map_size_x+1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xF00C4FFFFFF)!=(col&0xF00C4FFFFFF) || checked_voxels2[1][y+a+(z+b-chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x+1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_y1) {
									len_z1++;
								} else {
									break;
								}
							}


							for(unsigned char a=0;a<(chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF00C4FFFFFF)==(col&0xF00C4FFFFFF) && checked_voxels2[1][y+(z+a-chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xF00C4FFFFFF)!=(col&0xF00C4FFFFFF) || checked_voxels2[1][y+b+(z+a-chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
										break;
									}
								}
								if(a==len_z2) {
									len_y2++;
								} else {
									break;
								}
							}

							unsigned char len_y, len_z;

							if(len_y1*len_z1>len_y2*len_z2) {
								len_y = len_y1;
								len_z = len_z1;
							} else {
								len_y = len_y2;
								len_z = len_z2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_y = len_z = 1;
							#endif

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(z+b-chunk_y)*map_size_y] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&4) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}
							
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
		for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
			for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
				unsigned long long col = map_colors[x+(y*map_size_z+z)*map_size_x];
				if(col!=0xFFFFFFFF) {
					unsigned char r = (col&0xFF);
					unsigned char g = ((col>>8)&0xFF);
					unsigned char b = ((col>>16)&0xFF);
					if(y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if(checked_voxels[0][(x-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0) {
							unsigned char len_x1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_x2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<(chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xFC1FFFFFF)==(col&0xFC1FFFFFF) && checked_voxels[0][(x+a-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xFC1FFFFFF)!=(col&0xFC1FFFFFF) || checked_voxels[0][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_x1) {
									len_z1++;
								} else {
									break;
								}
							}


							for(unsigned char a=0;a<(chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xFC1FFFFFF)==(col&0xFC1FFFFFF) && checked_voxels[0][(x-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xFC1FFFFFF)!=(col&0xFC1FFFFFF) || checked_voxels[0][(x+b-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+b+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_z2) {
									len_x2++;
								} else {
									break;
								}
							}

							unsigned char len_x, len_z;

							if(len_x1*len_z1>len_x2*len_z2) {
								len_x = len_x1;
								len_z = len_z1;
							} else {
								len_x = len_x2;
								len_z = len_z2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_x = len_z = 1;
							#endif

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[0][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&1) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}
							
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
						if(checked_voxels[1][(x-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0) {
							unsigned char len_x1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_x2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<(chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xF0C2FFFFFF)==(col&0xF0C2FFFFFF) && checked_voxels[1][(x+a-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xF0C2FFFFFF)!=(col&0xF0C2FFFFFF) || checked_voxels[1][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_x1) {
									len_z1++;
								} else {
									break;
								}
							}


							for(unsigned char a=0;a<(chunk_y+CHUNK_SIZE-z);a++) {
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xF0C2FFFFFF)==(col&0xF0C2FFFFFF) && checked_voxels[1][(x-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xF0C2FFFFFF)!=(col&0xF0C2FFFFFF) || checked_voxels[1][(x+b-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
										break;
									}
								}
								if(a==len_z2) {
									len_x2++;
								} else {
									break;
								}
							}

							unsigned char len_x, len_z;

							if(len_x1*len_z1>len_x2*len_z2) {
								len_x = len_x1;
								len_z = len_z1;
							} else {
								len_x = len_x2;
								len_z = len_z2;
							}
							#ifdef DISABLE_GREEDY_MESHING
							len_x = len_z = 1;
							#endif

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[1][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&2) {
								s = shadow_mask;
								chunk_shadow_data[chunk_shadow_index++] = 1;
							} else {
								chunk_shadow_data[chunk_shadow_index++] = 0;
							}
							
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

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glNewList(displaylist,GL_COMPILE);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,chunk_color_data);
	glVertexPointer(3,GL_SHORT,0,chunk_vertex_data);
	glDrawArrays(GL_QUADS,0,chunk_vertex_index/3);
	glEndList();
	
	if(settings.shadow_entities) {
		for(int k=0;k<chunk_color_index;k++) {
			if(!chunk_shadow_data[k/12]) {
				chunk_color_data[k] = (unsigned char)((float)chunk_color_data[k]*shadow_mask);
			}
		}
		
		glNewList(displaylist_shadowed,GL_COMPILE);
		glColorPointer(3,GL_UNSIGNED_BYTE,0,chunk_color_data);
		glVertexPointer(3,GL_SHORT,0,chunk_vertex_data);
		glDrawArrays(GL_QUADS,0,chunk_vertex_index/3);
		glEndList();
	}
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	free(chunk_vertex_data);
	free(chunk_color_data);
	free(chunk_shadow_data);
	return max_height;
}

void chunk_update_all() {
	/*if(chunk_geometry_rebuild) {
		for(unsigned char k=0;k<CHUNKS_PER_DIM;k++) {
			int chunk_x = (chunk_geometry_rebuild_state%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_geometry_rebuild_state/CHUNKS_PER_DIM)*CHUNK_SIZE;
			chunk_display_lists[chunk_geometry_rebuild_state] = glGenLists(1);
			chunk_display_lists_shadowed[chunk_geometry_rebuild_state] = glGenLists(1);
			chunk_max_height[chunk_geometry_rebuild_state] = chunk_generate(chunk_display_lists[chunk_geometry_rebuild_state], chunk_display_lists_shadowed[chunk_geometry_rebuild_state], chunk_x, chunk_y);
			chunk_last_update[chunk_geometry_rebuild_state] = timems();
			//printf("Generating chunks %.1f\n",((float)chunk_geometry_rebuild_state/(float)(CHUNKS_PER_DIM*CHUNKS_PER_DIM))*100.0F);
			chunk_geometry_rebuild_state++;
			if(chunk_geometry_rebuild_state==CHUNKS_PER_DIM*CHUNKS_PER_DIM) {
				chunk_geometry_rebuild = 0;
				break;
			}
		}
	}*/
	
	if(chunk_geometry_changed_lenght>0) {
		for(int k=0;k<chunk_geometry_changed_lenght;k++) {
			//printf("UPDATE GEOMETRY: %i %i\n",chunk_geometry_changed[k]%CHUNKS_PER_DIM,chunk_geometry_changed[k]/CHUNKS_PER_DIM);
			int chunk_x = (chunk_geometry_changed[k]%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_geometry_changed[k]/CHUNKS_PER_DIM)*CHUNK_SIZE;
			if(glIsList(chunk_display_lists[chunk_geometry_changed[k]])) {
				glDeleteLists(chunk_display_lists[chunk_geometry_changed[k]],1);
				glDeleteLists(chunk_display_lists_shadowed[chunk_geometry_changed[k]],1);
			}
			chunk_display_lists[chunk_geometry_changed[k]] = glGenLists(1);
			chunk_display_lists_shadowed[chunk_geometry_changed[k]] = glGenLists(1);
			chunk_max_height[chunk_geometry_changed[k]] = chunk_generate(chunk_display_lists[chunk_geometry_changed[k]], chunk_display_lists_shadowed[chunk_geometry_changed[k]], chunk_x, chunk_y);
			chunk_last_update[chunk_geometry_changed[k]] = timems();
			for(int l=0;l<((chunk_max_height[chunk_geometry_changed[k]]+(CHUNK_SIZE-1))/CHUNK_SIZE);l++) {
				int needs_update = chunk_geometry_changed[k]-CHUNKS_PER_DIM*(l+1);
				int j;
				for(j=0;j<chunk_lighting_changed_lenght;j++) {
					if(chunk_lighting_changed[j]==needs_update) {
						break;
					}
				}
				if(!(j<chunk_lighting_changed_lenght)) {
					chunk_lighting_changed[chunk_lighting_changed_lenght++] = needs_update;
				}
			}
		}
		chunk_geometry_changed_lenght = 0;
	}
	
	if(chunk_lighting_changed_lenght>0) {
		for(int k=0;k<chunk_lighting_changed_lenght;k++) {
			//printf("UPDATE LIGHTING: %i %i\n",chunk_lighting_changed[k]%CHUNKS_PER_DIM,chunk_lighting_changed[k]/CHUNKS_PER_DIM);
			int chunk_x = (chunk_lighting_changed[k]%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_lighting_changed[k]/CHUNKS_PER_DIM)*CHUNK_SIZE;
			if(glIsList(chunk_display_lists[chunk_lighting_changed[k]])) {
				glDeleteLists(chunk_display_lists[chunk_lighting_changed[k]],1);
				glDeleteLists(chunk_display_lists_shadowed[chunk_lighting_changed[k]],1);
			}
			chunk_display_lists[chunk_lighting_changed[k]] = glGenLists(1);
			chunk_display_lists_shadowed[chunk_lighting_changed[k]] = glGenLists(1);
			chunk_max_height[chunk_lighting_changed[k]] = chunk_generate(chunk_display_lists[chunk_lighting_changed[k]], chunk_display_lists_shadowed[chunk_lighting_changed[k]], chunk_x, chunk_y);
			chunk_last_update[chunk_lighting_changed[k]] = timems();
		}
		chunk_lighting_changed_lenght = 0;
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