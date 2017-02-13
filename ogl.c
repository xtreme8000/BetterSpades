#define min(x, y) ((x)<(y) ? (x) : (y))

float wireframe_x = 0, wireframe_y = 0, wireframe_z = 0;

int texture_checkerboard = 0;


void ogl_reshape(int width, int height) {
	glViewport(0,0,width,height);
	settings.window_width = width;
	settings.window_height = height;
}

void ogl_init() {
	settings.opengl14 = true;
	settings.color_correction = false;
	settings.multisamples = 0;
	settings.shadow_entities = false;
	settings.ambient_occlusion = true;
	settings.render_distance = 256.0F;

	glDisable(GL_TEXTURE_RECTANGLE);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glClearDepth(1.0F);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);

	#ifdef OS_WINDOWS
		glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)wglGetProcAddress("glPointParameterfv");
	#endif
	#ifdef OS_LINUX
		glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)glXGetProcAddress("glPointParameterfv");
	#endif

	if(!settings.opengl14) {
		#ifdef OS_WINDOWS
			glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
			glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
			glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
			glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
			glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
			glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
			glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
			glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
			glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
			glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
			glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
			glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		#endif
		#ifdef OS_LINUX
			glCreateShader = (PFNGLCREATESHADERPROC)glXGetProcAddress("glCreateShader");
			glShaderSource = (PFNGLSHADERSOURCEPROC)glXGetProcAddress("glShaderSource");
			glCompileShader = (PFNGLCOMPILESHADERPROC)glXGetProcAddress("glCompileShader");
			glCreateProgram = (PFNGLCREATEPROGRAMPROC)glXGetProcAddress("glCreateProgram");
			glAttachShader = (PFNGLATTACHSHADERPROC)glXGetProcAddress("glAttachShader");
			glLinkProgram = (PFNGLLINKPROGRAMPROC)glXGetProcAddress("glLinkProgram");
			glUseProgram = (PFNGLUSEPROGRAMPROC)glXGetProcAddress("glUseProgram");
			glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glXGetProcAddress("glGetUniformLocation");
			glUniform1f = (PFNGLUNIFORM1FPROC)glXGetProcAddress("glUniform1f");
			glUniform4f = (PFNGLUNIFORM4FPROC)glXGetProcAddress("glUniform4f");
			glUniform1i = (PFNGLUNIFORM1IPROC)glXGetProcAddress("glUniform1i");
			//glTexImage3D = (PFNGLTEXIMAGE3DPROC)glXGetProcAddress("glTexImage3D");
		#endif

		int shadera = glCreateShader(GL_VERTEX_SHADER);
		unsigned char* vertex = file_load("vertex.shader");
		unsigned char* fragment = file_load("fragment.shader");

		glShaderSource(shadera,1,(const GLchar* const*)&vertex,NULL);
		glCompileShader(shadera);

		int shaderb = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(shaderb,1,(const GLchar* const*)&fragment,NULL);
		glCompileShader(shaderb);

		int program = glCreateProgram();
		glAttachShader(program,shadera);
		glAttachShader(program,shaderb);
		glLinkProgram(program);
		glUseProgram(program);
		uniform_point_size = glGetUniformLocation(program,"point_size");
		uniform_near_plane_height = glGetUniformLocation(program,"near_plane_height");
		uniform_camera_x = glGetUniformLocation(program,"camera_x");
		uniform_camera_z = glGetUniformLocation(program,"camera_z");
		uniform_fog_distance = glGetUniformLocation(program,"fog_distance");
		uniform_map_size_x = glGetUniformLocation(program,"max_size_x");
		uniform_map_size_z = glGetUniformLocation(program,"max_size_z");
		uniform_fog_color = glGetUniformLocation(program,"fog_color");
		uniform_setting_color_correction = glGetUniformLocation(program,"setting_color_correction");
		uniform_draw_ui = glGetUniformLocation(program,"draw_ui");

		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	}

	if(settings.multisamples>0) {
		glEnable(GL_MULTISAMPLE);
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV,GL_NICEST);
	}

	//particle_init();

	if(!settings.opengl14) {
		texture_color_correction = genTexture(color_correction,16,16,16);
	}
	fps_last_update = timems();

	for(int k=0;k<128;k++) {
		overlay_textures_id[k] = 0;
		overlay_textures_data[k] = 0;
		kv6_name[k] = 0;
		kv6_model[k] = 0;
	}
	
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1,&texture_checkerboard);
	glBindTexture(GL_TEXTURE_2D,texture_checkerboard);
	unsigned char data[4] = {255,128,128,255};
	glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,2,2,0,GL_ALPHA,GL_UNSIGNED_BYTE,data);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D,0);
	glDisable(GL_TEXTURE_2D);
}

void ogl_display() {
	float fog_color[] = {0.5F,0.9098F,1.0F,1.0F};
	glClearColor(fog_color[0],fog_color[1],fog_color[2],fog_color[3]);


	if(settings.opengl14) {
		glFogi(GL_FOG_MODE,GL_LINEAR);
		glFogfv(GL_FOG_COLOR,fog_color);
		glFogf(GL_FOG_START,settings.render_distance*0.375F);
		glFogf(GL_FOG_END,settings.render_distance);
		glEnable(GL_FOG);
	} else {
		glDisable(GL_FOG);
		glUniform1f(uniform_fog_distance,settings.render_distance);
		glUniform1f(uniform_camera_x,camera_x);
		glUniform1f(uniform_camera_z,camera_z);
		glUniform1f(uniform_map_size_x,map_size_x);
		glUniform1f(uniform_map_size_z,map_size_z);
		glUniform4f(uniform_fog_color,fog_color[0],fog_color[1],fog_color[2],fog_color[3]);
		glUniform1i(uniform_setting_color_correction, settings.color_correction);
		glUniform1i(uniform_draw_ui, false);
	}

	if(!settings.opengl14 && settings.color_correction) {
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D,texture_color_correction);
	} else {
		if(!settings.opengl14) {
			glDisable(GL_TEXTURE_3D);
		}
	}

	chunk_update_all();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | (GL_STENCIL_BUFFER_BIT*settings.shadow_entities));

	if(settings.opengl14) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(camera_fov, ((float)settings.window_width)/((float)settings.window_height), 0.1F, settings.render_distance+CHUNK_SIZE*4.0F+128.0F);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(camera_x, camera_y, camera_z, camera_x+sin(camera_rot_x)*sin(camera_rot_y), camera_y+cos(camera_rot_y), camera_z+cos(camera_rot_x)*sin(camera_rot_y), 0.0F, 1.0F, 0.0F);
	} else {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(camera_fov, ((float)settings.window_width)/((float)settings.window_height), 0.1F, settings.render_distance+CHUNK_SIZE*4.0F);
		gluLookAt(camera_x, camera_y, camera_z, camera_x+sin(camera_rot_x)*sin(camera_rot_y), camera_y+cos(camera_rot_y), camera_z+cos(camera_rot_x)*sin(camera_rot_y), 0.0F, 1.0F, 0.0F);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	camera_ExtractFrustum();

	float per = 0.0F;
	if(!chunk_geometry_rebuild) {

		if(settings.shadow_entities) {
			drawScene(true);

			glEnable(GL_STENCIL_TEST);
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
			glStencilFunc(GL_ALWAYS,0,0);
			glCullFace(GL_FRONT);
			glStencilOp(GL_KEEP,GL_KEEP,GL_INCR_WRAP);
			//chunk_draw_shadow_volume(particles_vertices,vertex_index);
			glCullFace(GL_BACK);
			glStencilOp(GL_KEEP,GL_KEEP,GL_DECR_WRAP);
			//chunk_draw_shadow_volume(particles_vertices,vertex_index);
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
			glCullFace(GL_BACK);
			glStencilFunc(GL_EQUAL,0,255);
			glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

			per = drawScene(false);

			glDisable(GL_STENCIL_TEST);
		} else {
			per = drawScene(false);
		}

		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(0.0F,-500.0F);
		glDisable(GL_CULL_FACE);
		glLineWidth(1.0F);
		glBegin(GL_QUADS);
		glColor4f(1.0F,1.0F,1.0F,1.0F);
		glVertex3f(wireframe_x,wireframe_y,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y,wireframe_z+1);
		glVertex3f(wireframe_x,wireframe_y,wireframe_z+1);

		glVertex3f(wireframe_x,wireframe_y+1,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y+1,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y+1,wireframe_z+1);
		glVertex3f(wireframe_x,wireframe_y+1,wireframe_z+1);

		glVertex3f(wireframe_x,wireframe_y,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y,wireframe_z);
		glVertex3f(wireframe_x+1,wireframe_y+1,wireframe_z);
		glVertex3f(wireframe_x,wireframe_y+1,wireframe_z);

		glVertex3f(wireframe_x,wireframe_y,wireframe_z+1);
		glVertex3f(wireframe_x+1,wireframe_y,wireframe_z+1);
		glVertex3f(wireframe_x+1,wireframe_y+1,wireframe_z+1);
		glVertex3f(wireframe_x,wireframe_y+1,wireframe_z+1);
		glEnd();
		glEnable(GL_CULL_FACE);
		glPolygonOffset(0.0F,0.0F);
		glDisable(GL_POLYGON_OFFSET_LINE);
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	} else {
		per = (float)chunk_geometry_rebuild_state/(float)(CHUNKS_PER_DIM*CHUNKS_PER_DIM);
	}
	
	float dist_to_border = min(min(camera_x,map_size_x-camera_x),min(camera_z,map_size_z-camera_z));
	if(dist_to_border<12.0F) {
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glBindTexture(GL_TEXTURE_2D,texture_checkerboard);
		
		glColor4f(1.0F,1.0F,1.0F,(1.0F-dist_to_border/12.0F)*0.75F);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0F,0.0F);
		glVertex3f(-0.1F,-0.1F,-0.1F);
		glTexCoord2f(0.0F,map_size_y/4);
		glVertex3f(-0.1F,map_size_y+0.1F,-0.1F);
		glTexCoord2f(map_size_x/4,map_size_y/4);
		glVertex3f(map_size_x+0.1F,map_size_y+0.1F,-0.1F);
		glTexCoord2f(map_size_x/4,0.0F);
		glVertex3f(map_size_x+0.1F,-0.1F,-0.1F);
		
		glTexCoord2f(0.0F,0.0F);
		glVertex3f(-0.1F,-0.1F,-0.1F);
		glTexCoord2f(map_size_y/4,0.0F);
		glVertex3f(-0.1F,map_size_y+0.1F,-0.1F);
		glTexCoord2f(map_size_y/4,map_size_z/4);
		glVertex3f(-0.1F,map_size_y+0.1F,map_size_z+0.1F);
		glTexCoord2f(0.0F,map_size_z/4);
		glVertex3f(-0.1F,-0.1F,map_size_z+0.1F);
		
		glTexCoord2f(0.0F,0.0F);
		glVertex3f(-0.1F,-0.1F,map_size_z+0.1F);
		glTexCoord2f(0.0F,map_size_y/4);
		glVertex3f(-0.1F,map_size_y+0.1F,map_size_z+0.1F);
		glTexCoord2f(map_size_x/4,map_size_y/4);
		glVertex3f(map_size_x+0.1F,map_size_y+0.1F,map_size_z+0.1F);
		glTexCoord2f(map_size_x/4,0.0F);
		glVertex3f(map_size_x+0.1F,-0.1F,map_size_z+0.1F);
		
		glTexCoord2f(0.0F,0.0F);
		glVertex3f(map_size_x+0.1F,-0.1F,-0.1F);
		glTexCoord2f(map_size_y/4,0.0F);
		glVertex3f(map_size_x+0.1F,map_size_y+0.1F,-0.1F);
		glTexCoord2f(map_size_y/4,map_size_z/4);
		glVertex3f(map_size_x+0.1F,map_size_y+0.1F,map_size_z+0.1F);
		glTexCoord2f(0.0F,map_size_z/4);
		glVertex3f(map_size_x+0.1F,-0.1F,map_size_z+0.1F);
		glEnd();
		
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glEnable(GL_CULL_FACE);
	}

	float time_delta = ((float)(timems()-time_last_frame))/1000.0F;
	time_last_frame = timems();
	float fps = (1.0F/time_delta);
	if(fps>fps_max) {
		fps_max = fps;
	}
	if(fps<fps_min) {
		fps_min = fps;
	}
	if(timems()-fps_last_update>500) {
		for(int k=0;k<99;k++) {
			fps_last[k] = fps_last[k+1];
			per_last[k] = per_last[k+1];
		}
		fps_last[99] = (fps_max+fps_min)/2.0F;
		per_last[99] = per;
		fps_max = 0;
		fps_min = 1000000;
		fps_last_update = timems();
	}

	if(settings.opengl14) {
		glDisable(GL_FOG);
	} else {
		glUniform1i(uniform_draw_ui, true);
	}

	if(settings.opengl14) {
		glEnable(GL_FOG);
	} else {
		glUniform1i(uniform_draw_ui,false);
	}

	//particle_update(time_delta);

	glClear(GL_STENCIL_BUFFER_BIT);
}

void ogl_map_vxl_load(char* data, int x, int y, int z) {
	if(map_size_x*map_size_y*map_size_z<x*y*z) {
		map_colors = realloc(map_colors,x*y*z*sizeof(long long));
	}
	map_size_x = x;
	map_size_y = y;
	map_size_z = z;
	for(int x=0;x<map_size_x;x++) { for(int z=0;z<map_size_z;z++) { for(int y=0;y<map_size_y;y++) {
		map_colors[x+(y*map_size_z+z)*map_size_x] = 0xFFFFFFFF;
	} } }
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
	//particle_create(color,x,y,z,velocity,velocity_y,amount,min_size,max_size);
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

void ogl_set_wireframe(float x, float y, float z) {
	wireframe_x = x;
	wireframe_y = y;
	wireframe_z = z;
}

void ogl_render_sprite(char* filename, float x, float y, float z, int xsiz, int ysiz, int zsiz, float dx, float dy, float dz, float rx, float ry, float rz) {
	int free_slot = -1;
	kv6_t* model = 0;
	for(int k=0;k<128;k++) {
		if(free_slot==-1 && kv6_name[k]==0) {
			free_slot = k;
		}
		if(kv6_name[k]==(int)filename) {
			model = kv6_model[k];
			break;
		}
	}
	if(model==0) {
		if(free_slot<0) {
			return;
		}
		kv6_name[free_slot] = (int)filename;
		unsigned char* data = file_load(filename);
		kv6_model[free_slot] = kv6_load(data);
		free(data);
		model = kv6_model[free_slot];
	}
	glDisable(GL_MULTISAMPLE);
	glColor4f(1.0F,1.0F,1.0F,1.0F);
	glPushMatrix();
	glTranslatef(x,y,z);
	//glRotatef(rz,0.0F,0.0F,1.0F);
	glRotatef(ry,0.0F,1.0F,0.0F);
	glRotatef(rx,1.0F,0.0F,0.0F);
	float avg = (dx+dy+dz)/3.0F;
	update_point_size(avg);
	glScalef(avg,avg,avg);
	glTranslatef(0.0F,0.0F,zsiz);
	kv6_render(model,0,255,0);
	glPopMatrix();
	glEnable(GL_MULTISAMPLE);
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

void ogl_render_3d_box(float x, float y, float z, float size, unsigned int color) {
	glColor3f(((color>>16)&0xFF)/255.0F,((color>>8)&0xFF)/255.0F,(color&0xFF)/255.0F);
	glDisable(GL_MULTISAMPLE);
	update_point_size(size);
	glBegin(GL_POINTS);
	glVertex3f(x,y,z);
	glEnd();
	glEnable(GL_MULTISAMPLE);
}

void ogl_render_hole(float x, float y, float z, float size, unsigned int color, unsigned char side) {
	float r = ((color>>16)&0xFF)/255.0F;
	float g = ((color>>8)&0xFF)/255.0F;
	float b = (color&0xFF)/255.0F;

	if(side==4) {
		r *= 0.6F;
		g *= 0.6F;
		b *= 0.6F;
		glEnable(GL_STENCIL_TEST);


		glStencilFunc(GL_ALWAYS,1,0);
		glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
		glDepthMask(GL_FALSE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0F,-500.0F);

		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glTranslatef(x,y,z);
		glBegin(GL_QUADS);
		glVertex3f(0.0F,0.0F,0.0F);
		glVertex3f(0.0F,0.25F,0.0F);
		glVertex3f(0.25F,0.25F,0.0F);
		glVertex3f(0.25F,0.0F,0.0F);
		glEnd();
		glEnable(GL_CULL_FACE);
		glPopMatrix();

		glPolygonOffset(0.0F,0.0F);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		glDepthMask(GL_TRUE);
		glDisable(GL_DEPTH_TEST);
		glStencilFunc(GL_EQUAL,1,255);
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glTranslatef(x,y,z-0.05F);
		glBegin(GL_QUADS);

		//back
		glColor3f(r*0.6F,g*0.6F,b*0.6F);
		glVertex3f(0.0F,0.0F,0.0F);
		glVertex3f(0.0F,0.25F,0.0F);
		glVertex3f(0.25F,0.25F,0.0F);
		glVertex3f(0.25F,0.0F,0.0F);

		//down
		glColor3f(r*0.7F,g*0.7F,b*0.7F);
		glVertex3f(0.0F,0.0F,0.0F);
		glVertex3f(0.0F,0.0F,0.05F);
		glVertex3f(0.25F,0.0F,0.05F);
		glVertex3f(0.25F,0.0F,0.0F);

		//up
		glColor3f(r*0.7F,g*0.7F,b*0.7F);
		glVertex3f(0.0F,0.25F,0.0F);
		glVertex3f(0.0F,0.25F,0.05F);
		glVertex3f(0.25F,0.25F,0.05F);
		glVertex3f(0.25F,0.25F,0.0F);

		glColor3f(r*0.8F,g*0.8F,b*0.8F);
		glVertex3f(0.25F,0.0F,0.0F);
		glVertex3f(0.25F,0.0F,0.05F);
		glVertex3f(0.25F,0.25F,0.05F);
		glVertex3f(0.25F,0.25F,0.0F);

		glColor3f(r*0.8F,g*0.8F,b*0.8F);
		glVertex3f(0.0F,0.0F,0.0F);
		glVertex3f(0.0F,0.0F,0.05F);
		glVertex3f(0.0F,0.25F,0.05F);
		glVertex3f(0.0F,0.25F,0.0F);
		glEnd();
		glEnable(GL_CULL_FACE);
		glPopMatrix();



		glStencilFunc(GL_ALWAYS,0,0);
		glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
		glDepthMask(GL_FALSE);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0.0F,-500.0F);

		glDisable(GL_CULL_FACE);
		glPushMatrix();
		glTranslatef(x,y,z);
		glBegin(GL_QUADS);
		glVertex3f(0.0F,0.0F,0.0F);
		glVertex3f(0.0F,0.25F,0.0F);
		glVertex3f(0.25F,0.25F,0.0F);
		glVertex3f(0.25F,0.0F,0.0F);
		glEnd();
		glEnable(GL_CULL_FACE);
		glPopMatrix();

		glPolygonOffset(0.0F,0.0F);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
		glDepthMask(GL_TRUE);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
	}
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

int ogl_kv6_bind_fullness() {
	int r = 0;
	for(int k=0;k<128;k++) {
		if(kv6_name[k]!=0) {
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
