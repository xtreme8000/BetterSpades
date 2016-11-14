void ogl_reshape(int width, int height) {
	reshape(width,height);
}

void ogl_init() {
	init();
}

void ogl_display() {
	display();
}

void ogl_map_vxl_load_s(char* data) {
	map_vxl_load_s(data);
}

void ogl_chunk_rebuild_all() {
	chunk_rebuild_all();
}

void ogl_map_set(int x, int y, int z, long long color) {
	map_set(x,y,z,color);
}

long long ogl_map_get(int x, int y, int z) {
	return map_get(x,y,z);
}

void ogl_particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount, float min_size, float max_size) {
	particle_create(color,x,y,z,velocity,velocity_y,amount,min_size,max_size);
}

void ogl_display_min() {
	glClear(GL_COLOR_BUFFER_BIT);
}

int ogl_deprecation_state() {
	int r = 0;
	if(settings.opengl14 && strstr(glGetString(GL_EXTENSIONS),"GL_ARB_texture_non_power_of_two")==NULL) {
		r += 1;
	}
	if(settings.opengl14) {
		r += 2;
	}
	return r;
}

void ogl_render_sprite(float x, float y, float z, int xsiz, int ysiz, int zsiz, float dx, float dy, float dz, float rx, float ry, float rz) {
	AABB a;
	a.min_x = a.min_y = a.min_z = 0.0F;
	aabb_set_size(&a,xsiz*dx,ysiz*dy,zsiz*dz);
	glPushMatrix();
	glTranslatef(x,y,z);
	glRotatef(rz,0.0F,0.0F,1.0F);
	glRotatef(ry,0.0F,1.0F,0.0F);
	glRotatef(rx,1.0F,0.0F,0.0F);
	aabb_render(&a);
	glPopMatrix();
}

void ogl_overlay_setup() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0F,settings.window_width,0.0F,settings.window_height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
}

int ogl_overlay_bind_fullness() {
	int r = 0;
	for(int k=0;k<128;k++) {
		if(overlay_textures_data[k]!=0) {
			r++;
		}
	}
	return r;
}

void overlay_bind_texture(unsigned char* texture, int w, int h) {
	int free_slot = -1;
	int id = -1;
	for(int k=0;k<128;k++) {
		if(free_slot==-1 && overlay_textures_data[k]==0) {
			free_slot = k;
		}
		if(overlay_textures_data[k]==(int)texture) {
			id = overlay_textures_id[k];
			break;
		}
	}
	if(id==-1) {
		if(free_slot!=-1) {
			int tex;
			glGenTextures(1,&tex);
			glBindTexture(GL_TEXTURE_2D,tex);
			if(!settings.opengl14 || strstr(glGetString(GL_EXTENSIONS),"GL_ARB_texture_non_power_of_two")!=NULL) {
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_BGRA,GL_UNSIGNED_BYTE,texture);
			} else {
				int w2,h2;
				unsigned char l = 1;
				for(char k=0;k<10;k++) { //only go till 1024 and round down
					if(w>=l) {
						w2 = l;
					}
					if(h>=l) {
						h2 = l;
					}
					l <<= 1;
				}
				unsigned char* new_img = malloc(w2*h2*4);
				for(int y=0;y<h2;y++) {
					for(int x=0;x<w2;x++) {
						new_img[(x+y*w2)*4] = texture[(int)((x/(float)w2*w)+(y/(float)h2*h*w))*4];
						new_img[(x+y*w2)*4+1] = texture[(int)((x/(float)w2*w)+(y/(float)h2*h*w))*4+1];
						new_img[(x+y*w2)*4+2] = texture[(int)((x/(float)w2*w)+(y/(float)h2*h*w))*4+2];
						new_img[(x+y*w2)*4+3] = texture[(int)((x/(float)w2*w)+(y/(float)h2*h*w))*4+3];
					}
				}
				glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w2,h2,0,GL_BGRA,GL_UNSIGNED_BYTE,new_img);
				free(new_img);
			}
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
			overlay_textures_id[free_slot] = tex;
			overlay_textures_data[free_slot] = (int)texture;
		}
	} else {
		glBindTexture(GL_TEXTURE_2D,id);
	}
}

void ogl_overlay_rect(unsigned char* texture, int texture_width, int texture_height, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, int x, int y, int w, int h) {
	if((int)texture==0) {
		glDisable(GL_TEXTURE_2D);
	} else {
		overlay_bind_texture(texture,texture_width,texture_height);
	}
	glColor4ub(red,green,blue,alpha);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0F,0.0F);
	glVertex2f(x,settings.window_height-y);
	glTexCoord2f(1.0F,0.0F);
	glVertex2f(x+w,settings.window_height-y);
	glTexCoord2f(1.0F,1.0F);
	glVertex2f(x+w,settings.window_height-y-h);
	glTexCoord2f(0.0F,1.0F);
	glVertex2f(x,settings.window_height-y-h);
	glEnd();
	if((int)texture==0) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glBindTexture(GL_TEXTURE_2D,0);
	}
}

void ogl_overlay_rect_sub(unsigned char* texture, int texture_width, int texture_height, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha, int x, int y, int w, int h, int src_x, int src_y, int src_w, int src_h) {
	if((int)texture==0) {
		glDisable(GL_TEXTURE_2D);
	} else {
		overlay_bind_texture(texture,texture_width,texture_height);
	}
	glColor4ub(red,green,blue,alpha);
	glBegin(GL_QUADS);
	glTexCoord2f((float)src_x/(float)texture_width,(float)src_y/(float)texture_height);
	glVertex2f(x,settings.window_height-y);
	glTexCoord2f((float)(src_x+src_w)/(float)texture_width,(float)src_y/(float)texture_height);
	glVertex2f(x+w,settings.window_height-y);
	glTexCoord2f((float)(src_x+src_w)/(float)texture_width,(float)(src_y+src_h)/(float)texture_height);
	glVertex2f(x+w,settings.window_height-y-h);
	glTexCoord2f((float)src_x/(float)texture_width,(float)(src_y+src_h)/(float)texture_height);
	glVertex2f(x,settings.window_height-y-h);
	glEnd();
	if((int)texture==0) {
		glEnable(GL_TEXTURE_2D);
	} else {
		glBindTexture(GL_TEXTURE_2D,0);
	}
}

void ogl_overlay_finish() {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

const char* ogl_info(int i) {
	switch(i) {
		case 0:
			return glGetString(GL_VERSION);
		case 1:
			return glGetString(GL_VENDOR);
		case 2:
			return glGetString(GL_RENDERER);
	}
}

void ogl_camera_setup(float a, float b, float x, float y, float z) {
	camera_rot_x = a;
	camera_rot_y = b;
	camera_x = x;
	camera_y = y;
	camera_z = z;
}