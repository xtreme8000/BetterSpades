kv6_t kv6_load(unsigned char* bytes) {
	kv6_t ret;
	ret.has_display_list = 0;
	int index = 0;
	if(buffer_read32(bytes,index)==0x6C78764B) { //"Kvxl"
		index += 4;
		ret.xsiz = buffer_read32(bytes,index);
		index += 4;
		ret.ysiz = buffer_read32(bytes,index);
		index += 4;
		ret.zsiz = buffer_read32(bytes,index);
		index += 4;
		ret.color = malloc(ret.xsiz*ret.ysiz*ret.zsiz*4);
		memset(ret.color,0,ret.xsiz*ret.ysiz*ret.zsiz*4);
		//float xpiv = fread32f(ptr_myfile);
		//float ypiv = fread32f(ptr_myfile);
		//float zpiv = fread32f(ptr_myfile);
		index += 12;
		int blklen = buffer_read32(bytes,index);
		index += 4;
		unsigned int* blkdata_color = malloc(blklen*4);
		unsigned short* blkdata_zpos = malloc(blklen*2);
		unsigned char* blkdata_visfaces = malloc(blklen);
		unsigned char* blkdata_lighting = malloc(blklen);
		for(int k=0;k<blklen;k++) {
			blkdata_color[k] = buffer_read32(bytes,index);
			index += 4;
			blkdata_zpos[k] = buffer_read16(bytes,index);
			index += 2;
			blkdata_visfaces[k] = buffer_read8(bytes,index++);
			blkdata_lighting[k] = buffer_read8(bytes,index++);
		}
		index += 4*ret.xsiz;
		int blkdata_offset = 0;
		for(int x=0;x<ret.xsiz;x++) {
			for(int y=0;y<ret.ysiz;y++) {
				int size = buffer_read16(bytes,index);
				index += 2;
				for(int z=0;z<size;z++) {
					ret.color[x+y*ret.xsiz+blkdata_zpos[blkdata_offset]*ret.xsiz*ret.ysiz] = (blkdata_color[blkdata_offset]&0xFFFFFF)|(blkdata_lighting[blkdata_offset]<<24);
					blkdata_offset++;
				}
			}
		}
		free(blkdata_color);
		free(blkdata_zpos);
		free(blkdata_visfaces);
		free(blkdata_lighting);
	}
	return ret;
}

void kv6_render(kv6_t* kv6, unsigned char red, unsigned char green, unsigned char blue) {
	if(!kv6->has_display_list) {
		int size = 0;
		for(int x=0;x<kv6->xsiz;x++) {
			for(int y=0;y<kv6->ysiz;y++) {
				for(int z=0;z<kv6->zsiz;z++) {
					if(kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]!=0) {
						size++;
					}
				}
			}
		}

		kv6->display_list_colors = malloc(size*3);
		kv6->display_list_vertices = malloc(size*3*2);

		int index_color = 0;
		int index_vertex = 0;
		for(int x=0;x<kv6->xsiz;x++) {
			for(int y=0;y<kv6->ysiz;y++) {
				for(int z=0;z<kv6->zsiz;z++) {
					if(kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]!=0) {
						int b = kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz] & 0xFF;
						int g = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>8) & 0xFF;
						int r = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>16) & 0xFF;
						int a = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>24) & 0xFF;
						if(r==0 && g==0 && b==0) {
							r = red;
							g = green;
							b = blue;
						}
						kv6->display_list_colors[index_color++] = (unsigned char)((r/255.0F*(a/255.0F*0.5F+0.5F))*255.0F);
						kv6->display_list_colors[index_color++] = (unsigned char)((g/255.0F*(a/255.0F*0.5F+0.5F))*255.0F);
						kv6->display_list_colors[index_color++] = (unsigned char)((b/255.0F*(a/255.0F*0.5F+0.5F))*255.0F);

						kv6->display_list_vertices[index_vertex++] = x;
						kv6->display_list_vertices[index_vertex++] = kv6->zsiz-z;
						kv6->display_list_vertices[index_vertex++] = -y;
					}
				}
			}
		}

		kv6->display_list = glGenLists(1);

		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glNewList(kv6->display_list,GL_COMPILE);
		glVertexPointer(3,GL_SHORT,0,kv6->display_list_vertices);
		glColorPointer(3,GL_UNSIGNED_BYTE,0,kv6->display_list_colors);
		glDrawArrays(GL_POINTS,0,index_vertex/3);
		glEndList();
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);

		free(kv6->display_list_vertices);
		free(kv6->display_list_colors);
		kv6->has_display_list = 1;
	}

	glCallList(kv6->display_list);

	/*glBegin(GL_QUADS);
	unsigned char b = 0;
	for(int x=0;x<kv6->xsiz;x++) {
		for(int y=0;y<kv6->ysiz;y++) {
			for(int z=0;z<kv6->zsiz;z++) {
				if(kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]!=0 /*&& b==0) {
					/*b = 1;
					float sun_x = 3.142F;
					float sun_y = 2.356F;

					float ray_x = sin(sun_x)*sin(sun_y);
					float ray_y = cos(sun_y);
					float ray_z = cos(sun_x)*sin(sun_y);

					float plane_x = sin(sun_x)*sin(sun_y-1.57F);
					float plane_y = cos(sun_y-1.57F);
					float plane_z = cos(sun_x)*sin(sun_y-1.57F);

					/*glColor4f(1.0F,1.0F,0.0F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+ray_x*512.0F,kv6->zsiz-z+ray_y*512.0F,-y+ray_z*512.0F);
					glColor4f(1.0F,0.0F,0.0F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+plane_x*2.0F,kv6->zsiz-z+plane_y*2.0F,-y+plane_z*2.0F);*/

					/*float cross_x = ray_y*plane_z-ray_z*plane_y;
					float cross_y = ray_z*plane_x-ray_x*plane_z;
					float cross_z = ray_x*plane_y-ray_y*plane_x;

					/*glColor4f(0.0F,0.0F,1.0F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+cross_x*2.0F,kv6->zsiz-z+cross_y*2.0F,-y+cross_z*2.0F);*/

					/*glColor4f(0.0F,0.0F,0.0F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+cross_x,kv6->zsiz-z+cross_y,-y+cross_z);
					glVertex3f(x+cross_x+plane_x,kv6->zsiz-z+cross_y+plane_y,-y+cross_z+plane_z);
					glVertex3f(x+plane_x,kv6->zsiz-z+plane_y,-y+plane_z);

					glColor4f(0.25F,0.25F,0.25F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+ray_x*512.0F,kv6->zsiz-z+ray_y*512.0F,-y+ray_z*512.0F);
					glVertex3f(x+cross_x+ray_x*512.0F,kv6->zsiz-z+cross_y+ray_y*512.0F,-y+cross_z+ray_z*512.0F);
					glVertex3f(x+cross_x,kv6->zsiz-z+cross_y,-y+cross_z);

					glColor4f(0.25F,0.25F,0.25F,1.0F);
					glVertex3f(x+plane_x,kv6->zsiz-z+plane_y,-y+plane_z);
					glVertex3f(x+cross_x+plane_x,kv6->zsiz-z+cross_y+plane_y,-y+cross_z+plane_z);
					glVertex3f(x+cross_x+ray_x*512.0F+plane_x,kv6->zsiz-z+cross_y+ray_y*512.0F+plane_y,-y+cross_z+ray_z*512.0F+plane_z);
					glVertex3f(x+ray_x*512.0F+plane_x,kv6->zsiz-z+ray_y*512.0F+plane_y,-y+ray_z*512.0F+plane_z);

					glColor4f(0.5F,0.5F,0.5F,1.0F);
					glVertex3f(x,kv6->zsiz-z,-y);
					glVertex3f(x+plane_x,kv6->zsiz-z+plane_y,-y+plane_z);
					glVertex3f(x+ray_x*512.0F,kv6->zsiz-z+ray_y*512.0F,-y+ray_z*512.0F);
					glVertex3f(x+plane_x+ray_x*512.0F,kv6->zsiz-z+plane_y+ray_y*512.0F,-y+plane_z+ray_z*512.0F);

					glColor4f(0.5F,0.5F,0.5F,1.0F);
					glVertex3f(x+cross_x,kv6->zsiz-z+cross_y,-y+cross_z);
					glVertex3f(x+cross_x+plane_x+ray_x*512.0F,kv6->zsiz-z+cross_y+plane_y+ray_y*512.0F,-y+cross_z+plane_z+ray_z*512.0F);
					glVertex3f(x+cross_x+ray_x*512.0F,kv6->zsiz-z+cross_y+ray_y*512.0F,-y+cross_z+ray_z*512.0F);
					glVertex3f(x+cross_x+plane_x,kv6->zsiz-z+cross_y+plane_y,-y+cross_z+plane_z);
				}
			}
		}
	}
	glEnd();*/
}