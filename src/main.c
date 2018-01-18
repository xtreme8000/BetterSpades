#include "common.h"

int ms_seed = 1;
int ms_rand() {
  ms_seed = ms_seed*0x343FD+0x269EC3;
  return (ms_seed>>0x10) & 0x7FFF;
}

int chat_input_mode = CHAT_NO_INPUT;

char chat[2][10][256] = {0}; //chat[0] is current input
unsigned int chat_color[2][10];
float chat_timer[2][10];
void chat_add(int channel, unsigned int color, const char* msg) {
	for(int k=9;k>1;k--) {
		strcpy(chat[channel][k],chat[channel][k-1]);
		chat_color[channel][k] = chat_color[channel][k-1];
		chat_timer[channel][k] = chat_timer[channel][k-1];
	}
	strcpy(chat[channel][1],msg);
	chat_color[channel][1] = color;
	chat_timer[channel][1] = glfwGetTime();
}
char chat_popup[256] = {};
float chat_popup_timer = 0.0F;

void chat_showpopup(const char* msg) {
	strcpy(chat_popup,msg);
	chat_popup_timer = glfwGetTime();
}

void drawScene(float dt) {
	if(settings.ambient_occlusion) {
		glShadeModel(GL_SMOOTH);
	} else {
		glShadeModel(GL_FLAT);
	}

	matrix_upload();
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	chunk_draw_visible();
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glShadeModel(GL_FLAT);
	matrix_upload();
	particle_render();
	tracer_render();

	map_damaged_voxels_render();
	matrix_upload();

	if(gamestate.gamemode_type==GAMEMODE_CTF) {
		if(!gamestate.gamemode.ctf.team_1_intel) {
			matrix_push();
			matrix_translate(gamestate.gamemode.ctf.team_1_intel_location.dropped.x,
						 63.0F-gamestate.gamemode.ctf.team_1_intel_location.dropped.z+1.0F,
						 gamestate.gamemode.ctf.team_1_intel_location.dropped.y);
			matrix_upload();
			kv6_render(&model_intel,TEAM_1);
			matrix_pop();
		}
		if(!gamestate.gamemode.ctf.team_2_intel) {
			matrix_push();
			matrix_translate(gamestate.gamemode.ctf.team_2_intel_location.dropped.x,
						 63.0F-gamestate.gamemode.ctf.team_2_intel_location.dropped.z+1.0F,
						 gamestate.gamemode.ctf.team_2_intel_location.dropped.y);
			matrix_upload();
			kv6_render(&model_intel,TEAM_2);
			matrix_pop();
		}
		matrix_push();
		matrix_translate(gamestate.gamemode.ctf.team_1_base.x,
					 63.0F-gamestate.gamemode.ctf.team_1_base.z+1.0F,
					 gamestate.gamemode.ctf.team_1_base.y);
		matrix_upload();
		kv6_render(&model_tent,TEAM_1);
		matrix_pop();
		matrix_push();
		matrix_translate(gamestate.gamemode.ctf.team_2_base.x,
					 63.0F-gamestate.gamemode.ctf.team_2_base.z+1.0F,
					 gamestate.gamemode.ctf.team_2_base.y);
		matrix_upload();
		kv6_render(&model_tent,TEAM_2);
		matrix_pop();
	}
	if(gamestate.gamemode_type==GAMEMODE_TC) {
		for(int k=0;k<gamestate.gamemode.tc.territory_count;k++) {
			matrix_push();
			matrix_translate(gamestate.gamemode.tc.territory[k].x,
						 63.0F-gamestate.gamemode.tc.territory[k].z+1.0F,
						 gamestate.gamemode.tc.territory[k].y);
			matrix_upload();
			kv6_render(&model_tent,min(gamestate.gamemode.tc.territory[k].team,2));
			matrix_pop();
		}
	}
}

void glxcheckErrors(char* file, int line) {

}

float last_cy;
void display(float dt) {
	if(network_map_transfer) {
		glClearColor(0.0F,0.0F,0.0F,1.0F);
	} else {
		glClearColor(fog_color[0],fog_color[1],fog_color[2],fog_color[3]);
	}

	if(hud_active->render_world) {
		glEnable(GL_DEPTH_TEST);
		glDepthRange(0.0F,1.0F);

		chunk_update_all();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | (GL_STENCIL_BUFFER_BIT*settings.shadow_entities));

		player_move(&players[local_player_id],dt,local_player_id);
		last_cy = players[local_player_id].physics.eye.y-players[local_player_id].physics.velocity.y*0.5F;

		//following two if-statements disable smooth crouching on local player
		if(camera_mode==CAMERAMODE_FPS) {
			if(!players[local_player_id].input.keys.crouch && key_map[GLFW_KEY_LEFT_CONTROL] && !players[local_player_id].physics.airborne) {
				players[local_player_id].pos.y -= 0.9F;
				players[local_player_id].physics.eye.y -= 0.9F;
				last_cy -= 0.9F;
			}
		}

		float fov = camera_fov;
		if(camera_mode==CAMERAMODE_FPS && players[local_player_id].held_item==TOOL_GUN && players[local_player_id].input.buttons.rmb) {
			fov *= 0.75F;
		}

		if(settings.opengl14) {
			matrix_select(matrix_projection);
			matrix_identity();
			matrix_perspective(fov, ((float)settings.window_width)/((float)settings.window_height), 0.1F, settings.render_distance+CHUNK_SIZE*4.0F+128.0F);
			matrix_upload_p();

			matrix_select(matrix_view);
			matrix_identity();
			camera_apply(dt);
			matrix_select(matrix_model);
			matrix_identity();

			float lpos[4] = {0.0F,-1.0F,1.0F,0.0F};
			float lambient[4] = {0.5F,0.5F,0.5F,1.0F};
			float ldiffuse[4] = {0.5F,0.5F,0.5F,1.0F};
			matrix_upload();
			glLightfv(GL_LIGHT0,GL_POSITION,lpos);
			glLightfv(GL_LIGHT0,GL_AMBIENT,lambient);
			glLightfv(GL_LIGHT0,GL_DIFFUSE,ldiffuse);

			map_sun[0] = 1.0F;
			map_sun[1] = -3.0F;
			map_sun[2] = 1.0F;
			map_sun[3] = 0.0F;
			matrix_push();
			matrix_load(matrix_view);
			matrix_vector(map_sun);
			matrix_pop();
			float map_sun_len = sqrt(map_sun[0]*map_sun[0]+map_sun[1]*map_sun[1]+map_sun[2]*map_sun[2]);
			map_sun[0] /= map_sun_len;
			map_sun[1] /= map_sun_len;
			map_sun[2] /= map_sun_len;
		}

		camera_ExtractFrustum();

		float fps = 1.0F/dt;
		//printf("FPS: %0.2f\n",fps);

		if(!network_map_transfer) {

			glx_enable_sphericalfog();
			drawScene(dt);

			grenade_update(dt);
			tracer_update(dt);

			if(players[local_player_id].items_show && glfwGetTime()-players[local_player_id].items_show_start>=0.5F) {
				players[local_player_id].items_show = 0;
			}

			if(camera_mode==CAMERAMODE_FPS) {
				weapon_update();
				if(players[local_player_id].input.buttons.lmb
					&& players[local_player_id].held_item==TOOL_BLOCK
					&& (glfwGetTime()-players[local_player_id].item_showup)>=0.5F
					&& local_player_blocks>0) {
					int* pos = camera_terrain_pick(0);
					if(pos!=NULL && pos[1]>1 && distance3D(camera_x,camera_y,camera_z,pos[0],pos[1],pos[2])<5.0F*5.0F
						&& !(pos[0]==(int)camera_x && pos[1]==(int)camera_y+0 && pos[2]==(int)camera_z)
						&& !(pos[0]==(int)camera_x && pos[1]==(int)camera_y-1 && pos[2]==(int)camera_z)) {
						players[local_player_id].item_showup = glfwGetTime();
						local_player_blocks = max(local_player_blocks-1,0);

						struct PacketBlockAction blk;
						blk.player_id = local_player_id;
						blk.action_type = ACTION_BUILD;
						blk.x = pos[0];
						blk.y = pos[2];
						blk.z = 63-pos[1];
						network_send(PACKET_BLOCKACTION_ID,&blk,sizeof(blk));
						//read_PacketBlockAction(&blk,sizeof(blk));
					}
				}
				if(players[local_player_id].input.buttons.lmb
				   && players[local_player_id].held_item==TOOL_GRENADE
				   && glfwGetTime()-players[local_player_id].input.buttons.lmb_start>3.0F) {
					local_player_grenades = max(local_player_grenades-1,0);
					struct PacketGrenade g;
					g.player_id = local_player_id;
					g.x = players[local_player_id].pos.x;
					g.y = players[local_player_id].pos.z;
					g.z = 63.0F-players[local_player_id].pos.y;
					g.fuse_length = g.vx = g.vy = g.vz = 0.0F;
					network_send(PACKET_GRENADE_ID,&g,sizeof(g));
					read_PacketGrenade(&g,sizeof(g));
					players[local_player_id].input.buttons.lmb_start = glfwGetTime();
				}
			}

			int* pos;
			switch(players[local_player_id].held_item) {
				case TOOL_BLOCK:
					if(!players[local_player_id].input.keys.sprint)
						pos = camera_terrain_pick(0);
					break;
				default:
					pos = NULL;
			}
			if(pos!=NULL && pos[1]>1 && (pow(pos[0]-camera_x,2)+pow(pos[1]-camera_y,2)+pow(pos[2]-camera_z,2))<5*5) {
				matrix_upload();
				glColor3f(1.0F,0.0F,0.0F);
				glLineWidth(1.0F);
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_FALSE);
				struct Point cubes[64];
				int amount = 0;
				if(local_player_drag_active && players[local_player_id].input.buttons.rmb && players[local_player_id].held_item==TOOL_BLOCK) {
					amount = map_cube_line(local_player_drag_x,local_player_drag_z,63-local_player_drag_y,pos[0],pos[2],63-pos[1],cubes);
				} else {
					amount = 1;
					cubes[0].x = pos[0];
					cubes[0].y = pos[2];
					cubes[0].z = 63-pos[1];
				}
				while(amount>0) {
					int tmp = cubes[amount-1].y;
					cubes[amount-1].y = 63-cubes[amount-1].z;
					cubes[amount-1].z = tmp;
					if(amount<=local_player_blocks) {
						glColor3f(1.0F,1.0F,1.0F);
					}
					glBegin(GL_LINES);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1);

					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1);

					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1);
					glVertex3s(cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1);
					glEnd();
					amount--;
				}
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

			if(glfwGetTime()-players[local_player_id].item_disabled<0.3F) {
				players[local_player_id].item_showup = glfwGetTime();
				if(players[local_player_id].input.buttons.lmb) {
					players[local_player_id].input.buttons.lmb_start = glfwGetTime();
				}
				players[local_player_id].input.buttons.rmb = 0;
			} else {
				if(hud_active->render_localplayer) {
					float tmp2 = players[local_player_id].physics.eye.y;
					players[local_player_id].physics.eye.y = last_cy;
					if(camera_mode==CAMERAMODE_FPS) {
						glDepthRange(0.0F,0.05F);
					}
					matrix_select(matrix_projection);
					matrix_push();
					matrix_translate(0.0F,-0.25F,0.0F);
					matrix_upload_p();
					matrix_select(matrix_model);
					player_render(&players[local_player_id],local_player_id,NULL,1);
					matrix_select(matrix_projection);
					matrix_pop();
					matrix_select(matrix_model);
					glDepthRange(0.0F,1.0F);
					players[local_player_id].physics.eye.y = tmp2;
				}
			}


			matrix_upload_p();
			matrix_upload();
			player_update(dt);

			matrix_upload();
			map_collapsing_render(dt);
			matrix_upload();

			if(map_get(camera_x,camera_y,camera_z)!=0xFFFFFFFF) {
				glClear(GL_COLOR_BUFFER_BIT);
			}

			glx_disable_sphericalfog();
		}
	}

	if(hud_active->render_3D)
		hud_active->render_3D();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_MULTISAMPLE);
	matrix_select(matrix_projection);
	matrix_identity();
	matrix_ortho(0.0F,settings.window_width,0.0F,settings.window_height,-1.0F,1.0F);
	matrix_select(matrix_view);
	matrix_identity();
	matrix_select(matrix_model);
	matrix_identity();
	matrix_upload();
	matrix_upload_p();
	float scalex = settings.window_width/800.0F;
	float scalef = settings.window_height/600.0F;

	if(hud_active->render_2D)
		hud_active->render_2D(scalex,scalef);

	if(settings.multisamples>0) {
		glEnable(GL_MULTISAMPLE);
	}
}

void init(GLFWwindow* window) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glClearDepth(1.0F);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_FOG);

	glxcheckErrors(__FILE__,__LINE__);

	map_colors = malloc(map_size_x*map_size_y*map_size_z*sizeof(unsigned long long));
	memset(map_colors,(unsigned long long)0xFFFFFFFF,map_size_x*map_size_y*map_size_z);

	map_minimap = malloc(map_size_x*map_size_z*sizeof(unsigned char)*4);
	//set minimap borders (white on 64x64 chunks, black map border)
	memset(map_minimap,0xCCCCCCFF,map_size_x*map_size_z*sizeof(unsigned char)*4);

    chunk_init();
	font_init();
	player_init();
	particle_init();
	network_init();
	kv6_init();
	texture_init();
	sound_init();
	tracer_init();
	hud_init(window);

	weapon_set();
}

void reshape(GLFWwindow* window, int width, int height) {
	glViewport(0,0,width,height);
	settings.window_width = width;
	settings.window_height = height;
	glfwSwapInterval(settings.vsync);
}

char text_input_first;
void text_input(GLFWwindow* window, unsigned int codepoint) {
	if(chat_input_mode==CHAT_NO_INPUT) {
		return;
	}
	if(text_input_first) {
		text_input_first = 0;
		return;
	}

	int len = strlen(chat[0][0]);
	if(len<128) {
		chat[0][0][len] = codepoint;
		chat[0][0][len+1] = 0;
	}
}

void keys(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(hud_active->input_keyboard)
		hud_active->input_keyboard(key,action,mods);

	if(key==GLFW_KEY_F11 && action==GLFW_PRESS) { //switch between fullscreen
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if(!settings.fullscreen) {
			glfwSetWindowMonitor(window,glfwGetPrimaryMonitor(),0,0,mode->width,mode->height,mode->refreshRate);
			settings.fullscreen = 1;
		} else {
			glfwSetWindowMonitor(window,NULL,(mode->width-800)/2,(mode->height-600)/2,800,600,0);
			settings.fullscreen = 0;
		}
	}

	if(key==GLFW_KEY_F5 && action==GLFW_PRESS) { //take screenshot
		time_t pic_time;
		time(&pic_time);
		char pic_name[128];
		sprintf(pic_name,"screenshots/%i.png",pic_time);

		unsigned char* pic_data = malloc(settings.window_width*settings.window_height*4*2);
		glReadPixels(0,0,settings.window_width,settings.window_height,GL_RGBA,GL_UNSIGNED_BYTE,pic_data);

		for(int y=0;y<settings.window_height;y++) { //mirror image (top-bottom)
			memcpy(pic_data+settings.window_width*4*(y+settings.window_height),
				   pic_data+settings.window_width*4*(settings.window_height-y-1),
				   settings.window_width*4);
		}

		lodepng_encode32_file(pic_name,pic_data+settings.window_width*settings.window_height*4,settings.window_width,settings.window_height);
		free(pic_data);

		sprintf(pic_name,"Saved screenshot as screenshots/%i.png",pic_time);
		chat_add(0,0x0000FF,pic_name);
	}
}

void mouse_click(GLFWwindow* window, int button, int action, int mods) {
	if(hud_active->input_mouseclick)
		hud_active->input_mouseclick(button,action,mods);
}

void mouse(GLFWwindow* window, double x, double y) {
	if(hud_active->input_mouselocation)
		hud_active->input_mouselocation(x,y);
}

void mouse_scroll(GLFWwindow* window, double xoffset, double yoffset) {
	if(hud_active->input_mousescroll)
		hud_active->input_mousescroll(yoffset);
}

void deinit() {
	if(network_connected)
		network_disconnect();
}

int main(int argc, char** argv) {
	settings.opengl14 = true;
	settings.color_correction = false;
	settings.multisamples = 0;
	settings.shadow_entities = false;
	settings.ambient_occlusion = false;
	settings.render_distance = 128.0F;
	settings.window_width = 800;
	settings.window_height = 600;
	settings.player_arms = 0;
	settings.fullscreen = 0;
	settings.greedy_meshing = 0;
	settings.mouse_sensitivity = MOUSE_SENSITIVITY;
	strcpy(settings.name,"DEV_CLIENT");

	config_reload();


	glfwWindowHint(GLFW_VISIBLE,0);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
	if(!glfwInit()) {
		printf("GLFW3 init failed\n");
    	exit(1);
	}

	if(settings.multisamples>0) {
		glfwWindowHint(GLFW_SAMPLES,settings.multisamples);
	}

	GLFWwindow* window = glfwCreateWindow(settings.window_width,settings.window_height,"BetterSpades",settings.fullscreen?glfwGetPrimaryMonitor():NULL,NULL);
	if(!window) {
		printf("Could not open window\n");
		glfwTerminate();
		exit(1);
	}
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowPos(window,(mode->width-settings.window_width)/2.0F,(mode->height-settings.window_height)/2.0F);
	glfwShowWindow(window);

	GLFWimage icon;

	int error = lodepng_decode32_file(&icon.pixels,&icon.width,&icon.height,"icon.png");
    if(error) {
        printf("Could not load texture (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }
	glfwSetWindowIcon(window,1,&icon);

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window,reshape);
	glfwSetCursorPosCallback(window,mouse);
	glfwSetKeyCallback(window,keys);
	glfwSetMouseButtonCallback(window,mouse_click);
	glfwSetScrollCallback(window,mouse_scroll);
	glfwSetCharCallback(window,text_input);
	glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);

	printf("Vendor: %s\n",glGetString(GL_VENDOR));
	printf("Renderer: %s\n",glGetString(GL_RENDERER));
	printf("Version: %s\n",glGetString(GL_VERSION));

	if(settings.multisamples>0) {
		glEnable(GL_MULTISAMPLE);
		glHint(GL_MULTISAMPLE_FILTER_HINT_NV,GL_NICEST);
		int iMultiSample = 0;
		int iNumSamples = 0;
		glGetIntegerv(GL_SAMPLE_BUFFERS,&iMultiSample);
		glGetIntegerv(GL_SAMPLES,&iNumSamples);
		printf("MSAA on, GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d\n", iMultiSample, iNumSamples);
	}

	//for future reference
	/*if(!settings.opengl14) {
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
	}*/

	while(glGetError()!=GL_NO_ERROR);

	init(window);
	atexit(deinit);

	if(argc>1) {
		if(!strcmp(argv[1],"--help")) {
			printf("Usage: client                     [server browser]\n");
			printf("       client -aos://<ip>:<port>  [custom address]\n");
			exit(0);
		}

		if(!network_connect_string(argv[1]+1)) {
			printf("Error: Connection failed (use --help for instructions)\n");
			exit(1);
		} else {
			printf("Connection to %s successfull\n",argv[1]+1);
			hud_change(&hud_ingame);
		}
	}

	float last_frame;
	while(!glfwWindowShouldClose(window)) {
		float t = glfwGetTime();
		float dt = t-last_frame;
		last_frame = t;

		display(dt);

		particle_update(dt);
		sound_update();
		network_update();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}
