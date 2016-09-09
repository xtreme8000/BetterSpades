float chunk_draw_visible() {
	int chunks_to_draw_x[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
	int chunks_to_draw_y[CHUNKS_PER_DIM*CHUNKS_PER_DIM*2];
	int index = 0;
	int b = 0;
	for(char y=-9;y<CHUNKS_PER_DIM+9;y++) {
		for(char x=-9;x<CHUNKS_PER_DIM+9;x++) {
			if(((x*CHUNK_SIZE-camera_x)*(x*CHUNK_SIZE-camera_x)+(y*CHUNK_SIZE-camera_z)*(y*CHUNK_SIZE-camera_z))<(render_distance+CHUNK_SIZE*2.0F)*(render_distance+CHUNK_SIZE*2.0F)) {
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
	for(int k=0;k<index;k++) {
		chunk_render(chunks_to_draw_x[k]/CHUNK_SIZE,chunks_to_draw_y[k]/CHUNK_SIZE);
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

void chunk_render(int x, int y) {
	glPushMatrix();
	glTranslatef((x<0)*-map_size_x+(x>=CHUNKS_PER_DIM)*map_size_x,0.0F,(y<0)*-map_size_z+(y>=CHUNKS_PER_DIM)*map_size_z);
	if(x<0) {
		x += CHUNKS_PER_DIM;
	}
	if(y<0) {
		y += CHUNKS_PER_DIM;
	}
	if(x>=CHUNKS_PER_DIM) {
		x -= CHUNKS_PER_DIM;
	}
	if(y>=CHUNKS_PER_DIM) {
		y -= CHUNKS_PER_DIM;
	}

	if(chunk_render_mode) {
		glLineWidth(4.0F);
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
	}
	glCallList(chunk_display_lists[((y*CHUNKS_PER_DIM)|x)]);
	if(chunk_render_mode) {
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	}

	/*glBegin(GL_QUADS);
	glVertex3f(x*CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE+CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE+CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE+CHUNK_SIZE);
	glVertex3f(x*CHUNK_SIZE+CHUNK_SIZE,chunk_max_height[((y*CHUNKS_PER_DIM)|x)]+0.1F,y*CHUNK_SIZE);
	glEnd();*/

	glPopMatrix();
}

int chunk_generate(int displaylist, int chunk_x, int chunk_y) {
	float shadow_mask = 0.66F;
	
	int size = 0;
	int max_height = 0;
	for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
		for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
			for(int y=0;y<map_size_y;y++) {
				if(map_colors[x+(y*map_size_z+z)*map_size_x]!=0xFFFFFFFF) {
					if(max_height<y) {
						max_height = y;
					}
					unsigned char shadowed = 0;
					for(int a=0;a<map_size_y;a++) {
						if(map_get(x,y+a+1,z+a)!=0xFFFFFFFF) {
							shadowed |= 1;
						} else {
							if(map_get(x,y+a+1,z+a+1)!=0xFFFFFFFF || map_get(x,y+a+2,z+a)!=0xFFFFFFFF) {
								shadowed |= 1;
							}
						}
						if(map_get(x,y+a-1,z+a)!=0xFFFFFFFF) {
							shadowed |= 2;
						} else {
							if(map_get(x,y+a-1,z+a+1)!=0xFFFFFFFF || map_get(x,y+a,z+a)!=0xFFFFFFFF) {
								shadowed |= 2;
							}
						}
						if(map_get(x+1,y+a,z+a)!=0xFFFFFFFF) {
							shadowed |= 4;
						} else {
							if(map_get(x+1,y+a,z+a+1)!=0xFFFFFFFF || map_get(x+1,y+a+1,z+a)!=0xFFFFFFFF) {
								shadowed |= 4;
							}
						}
						if(map_get(x-1,y+a,z+a)!=0xFFFFFFFF) {
							shadowed |= 8;
						} else {
							if(map_get(x-1,y+a,z+a+1)!=0xFFFFFFFF || map_get(x-1,y+a+1,z+a)!=0xFFFFFFFF) {
								shadowed |= 8;
							}
						}
						if(map_get(x,y+a,z+a+1)!=0xFFFFFFFF) {
							shadowed |= 16;
						} else {
							if(map_get(x,y+a,z+a+2)!=0xFFFFFFFF || map_get(x,y+a+1,z+a+1)!=0xFFFFFFFF) {
								shadowed |= 16;
							}
						}
						if(map_get(x,y+a,z+a-1)!=0xFFFFFFFF) {
							shadowed |= 32;
						} else {
							if(map_get(x,y+a,z+a)!=0xFFFFFFFF || map_get(x,y+a+1,z+a-1)!=0xFFFFFFFF) {
								shadowed |= 32;
							}
						}
					}
					map_colors[x+(y*map_size_z+z)*map_size_x] &= 0x00FFFFFF;
					map_colors[x+(y*map_size_z+z)*map_size_x] |= (shadowed<<24);
					if(y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) { size++; }
					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) { size++; }
					if((x==0 && map_colors[map_size_x-1+(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z)*map_size_x-1]==0xFFFFFFFF)) { size++; }
					if((x==map_size_x-1 && map_colors[(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z)*map_size_x+1]==0xFFFFFFFF)) { size++; }
					if((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x]==0xFFFFFFFF)) { size++; }
					if((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x]==0xFFFFFFFF)) { size++; }
				}
			}
		}
	}
	max_height++;

	short* chunk_vertex_data = malloc(size*24);
	unsigned char* chunk_color_data = malloc(size*12);
	int chunk_vertex_index = 0;
	int chunk_color_index = 0;
	unsigned char checked_voxels[2][CHUNK_SIZE*CHUNK_SIZE];
	unsigned char checked_voxels2[2][CHUNK_SIZE*map_size_y];

	for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
		for(int k=0;k<CHUNK_SIZE*map_size_y;k++) {
			checked_voxels2[0][k] = 0;
			checked_voxels2[1][k] = 0;
		}
		for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
			for(int y=0;y<map_size_y;y++) {
				unsigned int col = map_colors[x+(y*map_size_z+z)*map_size_x];
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
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xE0FFFFFF)==(col&0xE0FFFFFF) && checked_voxels2[0][y+a+(x-chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xE0FFFFFF)!=(col&0xE0FFFFFF) || checked_voxels2[0][y+a+(x+b-chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+a)*map_size_z+map_size_z-1)*map_size_x+b]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+a)*map_size_z+z-1)*map_size_x+b]==0xFFFFFFFF))) {
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
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xE0FFFFFF)==(col&0xE0FFFFFF) && checked_voxels2[0][y+(x+a-chunk_x)*map_size_y]==0 && ((z==0 && map_colors[x+(y*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+(y*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xE0FFFFFF)!=(col&0xE0FFFFFF) || checked_voxels2[0][y+b+(x+a-chunk_x)*map_size_y]!=0 || !((z==0 && map_colors[x+((y+b)*map_size_z+map_size_z-1)*map_size_x+a]==0xFFFFFFFF) || (z>0 && map_colors[x+((y+b)*map_size_z+z-1)*map_size_x+a]==0xFFFFFFFF))) {
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

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(x+b-chunk_x)*map_size_y] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&32) {
								s = shadow_mask;
							}
							
							for(int k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.85F*s);
								chunk_color_data[chunk_color_index++] = (int)(g*0.85F*s);
								chunk_color_data[chunk_color_index++] = (int)(b*0.85F*s);
							}

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

					if((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x]==0xFFFFFFFF)) {
						if(checked_voxels2[1][y+(x-chunk_x)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_x1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_x2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xD0FFFFFF)==(col&0xD0FFFFFF) && checked_voxels2[1][y+a+(x-chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z)*map_size_x+b]&0xD0FFFFFF)!=(col&0xD0FFFFFF) || checked_voxels2[1][y+a+(x+b-chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+a)*map_size_z)*map_size_x+b]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+a)*map_size_z+z+1)*map_size_x+b]==0xFFFFFFFF))) {
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
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xD0FFFFFF)==(col&0xD0FFFFFF) && checked_voxels2[1][y+(x+a-chunk_x)*map_size_y]==0 && ((z==map_size_z-1 && map_colors[x+(y*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+(y*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
									len_x2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_x2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z)*map_size_x+a]&0xD0FFFFFF)!=(col&0xD0FFFFFF) || checked_voxels2[1][y+b+(x+a-chunk_x)*map_size_y]!=0 || !((z==map_size_z-1 && map_colors[x+((y+b)*map_size_z)*map_size_x+a]==0xFFFFFFFF) || (z<map_size_z-1 && map_colors[x+((y+b)*map_size_z+z+1)*map_size_x+a]==0xFFFFFFFF))) {
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

							for(unsigned char b=0;b<len_x;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(x+b-chunk_x)*map_size_y] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&16) {
								s = shadow_mask;
							}

							for(int k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.8F*s);
								chunk_color_data[chunk_color_index++] = (int)(g*0.8F*s);
								chunk_color_data[chunk_color_index++] = (int)(b*0.8F*s);
							}

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
				unsigned int col = map_colors[x+(y*map_size_z+z)*map_size_x];
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
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xC8FFFFFF)==(col&0xC8FFFFFF) && checked_voxels2[0][y+a+(z-chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z)*map_size_x-1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xC8FFFFFF)!=(col&0xC8FFFFFF) || checked_voxels2[0][y+a+(z+b-chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x-1]==0xFFFFFFFF))) {
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
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xC8FFFFFF)==(col&0xC8FFFFFF) && checked_voxels2[0][y+(z+a-chunk_y)*map_size_y]==0 && ((x==0 && map_colors[map_size_x-1+(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+(y*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xC8FFFFFF)!=(col&0xC8FFFFFF) || checked_voxels2[0][y+b+(z+a-chunk_y)*map_size_y]!=0 || !((x==0 && map_colors[map_size_x-1+((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x>0 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x-1]==0xFFFFFFFF))) {
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

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[0][y+a+(z+b-chunk_y)*map_size_y] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&8) {
								s = shadow_mask;
							}
							
							for(int k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.9F*s);
								chunk_color_data[chunk_color_index++] = (int)(g*0.9F*s);
								chunk_color_data[chunk_color_index++] = (int)(b*0.9F*s);
							}

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
					if((x==map_size_x-1 && map_colors[(y*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z)*map_size_x+1]==0xFFFFFFFF)) {
						if(checked_voxels2[1][y+(z-chunk_y)*map_size_y]==0) {
							unsigned char len_y1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_y2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<map_size_y-y;a++) {
								if((map_colors[x+((y+a)*map_size_z+z)*map_size_x]&0xC4FFFFFF)==(col&0xC4FFFFFF) && checked_voxels2[1][y+a+(z-chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z)*map_size_x+1]==0xFFFFFFFF))) {
									len_y1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_y1;a++) {
									if((map_colors[x+((y+a)*map_size_z+z+b)*map_size_x]&0xC4FFFFFF)!=(col&0xC4FFFFFF) || checked_voxels2[1][y+a+(z+b-chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+a)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+a)*map_size_z+z+b)*map_size_x+1]==0xFFFFFFFF))) {
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
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xC4FFFFFF)==(col&0xC4FFFFFF) && checked_voxels2[1][y+(z+a-chunk_y)*map_size_y]==0 && ((x==map_size_x-1 && map_colors[(y*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+(y*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<map_size_y-y;b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+((y+b)*map_size_z+z+a)*map_size_x]&0xC4FFFFFF)!=(col&0xC4FFFFFF) || checked_voxels2[1][y+b+(z+a-chunk_y)*map_size_y]!=0 || !((x==map_size_x-1 && map_colors[((y+b)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF) || (x<map_size_x-1 && map_colors[x+((y+b)*map_size_z+z+a)*map_size_x+1]==0xFFFFFFFF))) {
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

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_y;a++) {
									checked_voxels2[1][y+a+(z+b-chunk_y)*map_size_y] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&4) {
								s = shadow_mask;
							}

							for(int k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.95F*s);
								chunk_color_data[chunk_color_index++] = (int)(g*0.95F*s);
								chunk_color_data[chunk_color_index++] = (int)(b*0.95F*s);
							}

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
		for(int x=chunk_x;x<chunk_x+CHUNK_SIZE;x++) {
			for(int z=chunk_y;z<chunk_y+CHUNK_SIZE;z++) {
				unsigned int col = map_colors[x+(y*map_size_z+z)*map_size_x];
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
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xC1FFFFFF)==(col&0xC1FFFFFF) && checked_voxels[0][(x+a-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z)*map_size_x]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xC1FFFFFF)!=(col&0xC1FFFFFF) || checked_voxels[0][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+a+((y+1)*map_size_z+z+b)*map_size_x]==0xFFFFFFFF)) {
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
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xC1FFFFFF)==(col&0xC1FFFFFF) && checked_voxels[0][(x-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]==0 && (y==map_size_y-1 || map_colors[x+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xC1FFFFFF)!=(col&0xC1FFFFFF) || checked_voxels[0][(x+b-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]!=0 || !(y==map_size_y-1 || map_colors[x+b+((y+1)*map_size_z+z+a)*map_size_x]==0xFFFFFFFF)) {
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

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[0][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE] = 1;
								}
							}

							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&1) {
								s = shadow_mask;
							}
							
							for(unsigned char k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*s);
								chunk_color_data[chunk_color_index++] = (int)(g*s);
								chunk_color_data[chunk_color_index++] = (int)(b*s);
							}


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
					if(y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x]==0xFFFFFFFF) {
						if(checked_voxels[1][(x-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0) {
							unsigned char len_x1 = 0;
							unsigned char len_z1 = 1;

							unsigned char len_x2 = 1;
							unsigned char len_z2 = 0;

							for(unsigned char a=0;a<(chunk_x+CHUNK_SIZE-x);a++) {
								if((map_colors[x+(y*map_size_z+z)*map_size_x+a]&0xC2FFFFFF)==(col&0xC2FFFFFF) && checked_voxels[1][(x+a-chunk_x)+(z-chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_x1++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_y+CHUNK_SIZE-z);b++) {
								unsigned char a;
								for(a=0;a<len_x1;a++) {
									if((map_colors[x+(y*map_size_z+z+b)*map_size_x+a]&0xC2FFFFFF)!=(col&0xC2FFFFFF) || checked_voxels[1][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
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
								if((map_colors[x+(y*map_size_z+z+a)*map_size_x]&0xC2FFFFFF)==(col&0xC2FFFFFF) && checked_voxels[1][(x-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]==0 && (y>0 && map_colors[x+((y-1)*map_size_z+z)*map_size_x+a]==0xFFFFFFFF)) {
									len_z2++;
								} else {
									break;
								}
							}

							for(unsigned char b=1;b<(chunk_x+CHUNK_SIZE-x);b++) {
								unsigned char a;
								for(a=0;a<len_z2;a++) {
									if((map_colors[x+(y*map_size_z+z+a)*map_size_x+b]&0xC2FFFFFF)!=(col&0xC2FFFFFF) || checked_voxels[1][(x+b-chunk_x)+(z+a-chunk_y)*CHUNK_SIZE]!=0 || !(y>0 && map_colors[x+((y-1)*map_size_z+z+b)*map_size_x+a]==0xFFFFFFFF)) {
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

							for(unsigned char b=0;b<len_z;b++) {
								for(unsigned char a=0;a<len_x;a++) {
									checked_voxels[1][(x+a-chunk_x)+(z+b-chunk_y)*CHUNK_SIZE] = 1;
								}
							}
							
							float s = 1.0F;
							
							if((map_get(x,y,z)>>24)&2) {
								s = shadow_mask;
							}

							for(int k=0;k<4;k++) {
								chunk_color_data[chunk_color_index++] = (int)(r*0.5F*s);
								chunk_color_data[chunk_color_index++] = (int)(g*0.5F*s);
								chunk_color_data[chunk_color_index++] = (int)(b*0.5F*s);
							}

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

	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glNewList(displaylist,GL_COMPILE);
	glVertexPointer(3,GL_SHORT,0,chunk_vertex_data);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,chunk_color_data);
	glDrawArrays(GL_QUADS,0,chunk_vertex_index/3);
	glEndList();
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);

	free(chunk_vertex_data);
	free(chunk_color_data);
	return max_height;
}

void chunk_update_all() {
	if(chunk_geometry_rebuild) {
		for(unsigned char k=0;k<CHUNKS_PER_DIM;k++) {
			int chunk_x = (chunk_geometry_rebuild_state%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_geometry_rebuild_state/CHUNKS_PER_DIM)*CHUNK_SIZE;
			chunk_display_lists[chunk_geometry_rebuild_state] = glGenLists(1);
			chunk_max_height[chunk_geometry_rebuild_state] = chunk_generate(chunk_display_lists[chunk_geometry_rebuild_state], chunk_x, chunk_y);
			printf("Generating chunks %.1f\n",((float)chunk_geometry_rebuild_state/(float)(CHUNKS_PER_DIM*CHUNKS_PER_DIM))*100.0F);
			chunk_geometry_rebuild_state++;
			if(chunk_geometry_rebuild_state==CHUNKS_PER_DIM*CHUNKS_PER_DIM) {
				chunk_geometry_rebuild = 0;
				break;
			}
		}
	}
	
	if(chunk_geometry_changed_lenght>0) {
		for(int k=0;k<chunk_geometry_changed_lenght;k++) {
			//printf("UPDATE GEOMETRY: %i %i\n",chunk_geometry_changed[k]%CHUNKS_PER_DIM,chunk_geometry_changed[k]/CHUNKS_PER_DIM);
			int chunk_x = (chunk_geometry_changed[k]%CHUNKS_PER_DIM)*CHUNK_SIZE;
			int chunk_y = (chunk_geometry_changed[k]/CHUNKS_PER_DIM)*CHUNK_SIZE;
			glDeleteLists(chunk_display_lists[chunk_geometry_changed[k]],1);
			chunk_display_lists[chunk_geometry_changed[k]] = glGenLists(1);
			chunk_max_height[k] = chunk_generate(chunk_display_lists[chunk_geometry_changed[k]], chunk_x, chunk_y);
			for(int l=0;l<(chunk_max_height[k]/CHUNK_SIZE)+1;l++) {
				int needs_update =  chunk_geometry_changed[k]-CHUNKS_PER_DIM*(l+1);
				int j;
				for(j=0;j<chunk_lighting_changed_lenght;j++) {
					if(chunk_lighting_changed[j]==needs_update) {
						break;
					}
				}
				if(j<chunk_lighting_changed_lenght) {
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
			glDeleteLists(chunk_display_lists[chunk_lighting_changed[k]],1);
			chunk_display_lists[chunk_lighting_changed[k]] = glGenLists(1);
			chunk_max_height[k] = chunk_generate(chunk_display_lists[chunk_lighting_changed[k]], chunk_x, chunk_y);
		}
		chunk_lighting_changed_lenght = 0;
	}
}

void chunk_block_update(int x, int y, int z) {
	chunk_geometry_changed[chunk_geometry_changed_lenght++] = (z/CHUNK_SIZE)*CHUNKS_PER_DIM+(x/CHUNK_SIZE);
}