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

#ifdef OPENGL_ES
void glColor3f(float r, float g, float b) {
	glColor4f(r,g,b,1.0F);
}

void glColor3ub(unsigned char r, unsigned char g, unsigned char b) {
	glColor4ub(r,g,b,255);
}

void glDepthRange(float near, float far) {
	glDepthRangef(near,far);
}

void glClearDepth(float x) {
	glClearDepthf(x);
}
#endif

int fps = 0;

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
	chat_timer[channel][1] = window_time();
	if(channel==0)
		log_info(msg);
}
char chat_popup[256] = {};
int chat_popup_color;
float chat_popup_timer = 0.0F;
float chat_popup_duration = 0.0F;

void chat_showpopup(const char* msg, float duration, int color) {
	strcpy(chat_popup,msg);
	chat_popup_timer = window_time();
    chat_popup_duration = duration;
	chat_popup_color = color;
}

void drawScene() {
	if(settings.ambient_occlusion) {
		glShadeModel(GL_SMOOTH);
	} else {
		glShadeModel(GL_FLAT);
	}

	matrix_upload();
	chunk_draw_visible();

	if(settings.smooth_fog) {
		#ifdef OPENGL_ES
			glFogx(GL_FOG_MODE,GL_EXP2);
		#else
			glFogi(GL_FOG_MODE,GL_EXP2);
		#endif
		glFogf(GL_FOG_DENSITY,0.015F);
		glFogfv(GL_FOG_COLOR,fog_color);
		glEnable(GL_FOG);
	}

	glShadeModel(GL_FLAT);
	kv6_calclight(-1,-1,-1);
	matrix_upload();
	particle_render();
	tracer_render();

	map_damaged_voxels_render();
	matrix_upload();

	if(gamestate.gamemode_type==GAMEMODE_CTF) {
		if(!gamestate.gamemode.ctf.team_1_intel) {
			float x = gamestate.gamemode.ctf.team_1_intel_location.dropped.x;
			float y = 63.0F-gamestate.gamemode.ctf.team_1_intel_location.dropped.z+1.0F;
			float z = gamestate.gamemode.ctf.team_1_intel_location.dropped.y;
			matrix_push();
			matrix_translate(x,y,z);
			kv6_calclight(x,y,z);
			matrix_upload();
			kv6_render(&model_intel,TEAM_1);
			matrix_pop();
		}
		if(!gamestate.gamemode.ctf.team_2_intel) {
			float x = gamestate.gamemode.ctf.team_2_intel_location.dropped.x;
			float y = 63.0F-gamestate.gamemode.ctf.team_2_intel_location.dropped.z+1.0F;
			float z = gamestate.gamemode.ctf.team_2_intel_location.dropped.y;
			matrix_push();
			matrix_translate(x,y,z);
			kv6_calclight(x,y,z);
			matrix_upload();
			kv6_render(&model_intel,TEAM_2);
			matrix_pop();
		}
		if(map_object_visible(gamestate.gamemode.ctf.team_1_base.x,
						 63.0F-gamestate.gamemode.ctf.team_1_base.z+1.0F,
						 gamestate.gamemode.ctf.team_1_base.y)) {
			matrix_push();
			matrix_translate(gamestate.gamemode.ctf.team_1_base.x,
							 63.0F-gamestate.gamemode.ctf.team_1_base.z+1.0F,
							 gamestate.gamemode.ctf.team_1_base.y);
			kv6_calclight(gamestate.gamemode.ctf.team_1_base.x,
							 63.0F-gamestate.gamemode.ctf.team_1_base.z+1.0F,
							 gamestate.gamemode.ctf.team_1_base.y);
			matrix_upload();
			kv6_render(&model_tent,TEAM_1);
			matrix_pop();
		}
		if(map_object_visible(gamestate.gamemode.ctf.team_2_base.x,
						 63.0F-gamestate.gamemode.ctf.team_2_base.z+1.0F,
						 gamestate.gamemode.ctf.team_2_base.y)) {
			matrix_push();
			matrix_translate(gamestate.gamemode.ctf.team_2_base.x,
					 63.0F-gamestate.gamemode.ctf.team_2_base.z+1.0F,
					 gamestate.gamemode.ctf.team_2_base.y);
			kv6_calclight(gamestate.gamemode.ctf.team_2_base.x,
						 63.0F-gamestate.gamemode.ctf.team_2_base.z+1.0F,
						 gamestate.gamemode.ctf.team_2_base.y);
			matrix_upload();
			kv6_render(&model_tent,TEAM_2);
			matrix_pop();
		}
	}
	if(gamestate.gamemode_type==GAMEMODE_TC) {
		for(int k=0;k<gamestate.gamemode.tc.territory_count;k++) {
			matrix_push();
			matrix_translate(gamestate.gamemode.tc.territory[k].x,
						 63.0F-gamestate.gamemode.tc.territory[k].z+1.0F,
						 gamestate.gamemode.tc.territory[k].y);
			kv6_calclight(gamestate.gamemode.tc.territory[k].x,
 						 63.0F-gamestate.gamemode.tc.territory[k].z+1.0F,
 						 gamestate.gamemode.tc.territory[k].y);
			matrix_upload();
			kv6_render(&model_tent,min(gamestate.gamemode.tc.territory[k].team,2));
			matrix_pop();
		}
	}
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

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		player_move(&players[local_player_id],dt,local_player_id);
		last_cy = players[local_player_id].physics.eye.y-players[local_player_id].physics.velocity.y*0.4F;

		//following if-statement disables smooth crouching on local player
		if(camera_mode==CAMERAMODE_FPS && chat_input_mode==CHAT_NO_INPUT) {
			if(!players[local_player_id].input.keys.crouch && window_key_down(WINDOW_KEY_CROUCH) && !players[local_player_id].physics.airborne) {
				players[local_player_id].pos.y -= 0.9F;
				players[local_player_id].physics.eye.y -= 0.9F;
				last_cy -= 0.9F;
			}
		}

		if(settings.opengl14) {
			matrix_select(matrix_projection);
			matrix_identity();
			matrix_perspective(camera_fov_scaled(),((float)settings.window_width)/((float)settings.window_height), 0.1F, settings.render_distance+CHUNK_SIZE*4.0F+128.0F);
			matrix_upload_p();

			matrix_select(matrix_view);
			matrix_identity();
			camera_apply(dt);
			matrix_select(matrix_model);
			matrix_identity();

			matrix_upload();
			float lpos[4] = {0.0F,-1.0F,1.0F,0.0F};
			glLightfv(GL_LIGHT0,GL_POSITION,lpos);

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

		if(!network_map_transfer) {

			glx_enable_sphericalfog();
			drawScene();

			grenade_update(dt);
			tracer_update(dt);

			int render_fpv = (camera_mode==CAMERAMODE_FPS) || ((camera_mode==CAMERAMODE_BODYVIEW || camera_mode==CAMERAMODE_SPECTATOR) && cameracontroller_bodyview_mode);
			int is_local = (camera_mode==CAMERAMODE_FPS) || (cameracontroller_bodyview_player==local_player_id);
			int local_id = (camera_mode==CAMERAMODE_FPS)?local_player_id:cameracontroller_bodyview_player;

			if(players[local_player_id].items_show && window_time()-players[local_player_id].items_show_start>=0.5F) {
				players[local_player_id].items_show = 0;
			}

			if(camera_mode==CAMERAMODE_FPS) {
				weapon_update();
				if(players[local_player_id].input.buttons.lmb
					&& players[local_player_id].held_item==TOOL_BLOCK
					&& (window_time()-players[local_player_id].item_showup)>=0.5F
					&& local_player_blocks>0) {
					int* pos = camera_terrain_pick(0);
					if(pos!=NULL && pos[1]>1 && distance3D(camera_x,camera_y,camera_z,pos[0],pos[1],pos[2])<5.0F*5.0F
						&& !(pos[0]==(int)camera_x && pos[1]==(int)camera_y+0 && pos[2]==(int)camera_z)
						&& !(pos[0]==(int)camera_x && pos[1]==(int)camera_y-1 && pos[2]==(int)camera_z)) {
						players[local_player_id].item_showup = window_time();
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
				   && window_time()-players[local_player_id].input.buttons.lmb_start>3.0F) {
					local_player_grenades = max(local_player_grenades-1,0);
					struct PacketGrenade g;
					g.player_id = local_player_id;
					g.x = players[local_player_id].pos.x;
					g.y = players[local_player_id].pos.z;
					g.z = 63.0F-players[local_player_id].pos.y;
					g.fuse_length = g.vx = g.vy = g.vz = 0.0F;
					network_send(PACKET_GRENADE_ID,&g,sizeof(g));
					read_PacketGrenade(&g,sizeof(g));
					players[local_player_id].input.buttons.lmb_start = window_time();
				}
			}

			int* pos = NULL;
			switch(players[local_id].held_item) {
				case TOOL_BLOCK:
					if(!players[local_id].input.keys.sprint && render_fpv) {
						if(is_local)
							pos = camera_terrain_pick(0);
						else
							pos = camera_terrain_pickEx(0,camera_x,camera_y,camera_z,players[local_id].orientation_smooth.x,players[local_id].orientation_smooth.y,players[local_id].orientation_smooth.z);
					}
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
				if(is_local && local_player_drag_active && players[local_player_id].input.buttons.rmb && players[local_player_id].held_item==TOOL_BLOCK) {
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
					if(amount<=(is_local?local_player_blocks:50)) {
						glColor3f(1.0F,1.0F,1.0F);
					}

                    short vertices[72] = {
                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1,

                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1,

                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z,
                        cubes[amount-1].x+1,cubes[amount-1].y,cubes[amount-1].z+1,
                        cubes[amount-1].x+1,cubes[amount-1].y+1,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y,cubes[amount-1].z+1,
                        cubes[amount-1].x,cubes[amount-1].y+1,cubes[amount-1].z+1
                    };
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(3,GL_SHORT,0,vertices);
                    glDrawArrays(GL_LINES,0,24);
                    glDisableClientState(GL_VERTEX_ARRAY);
					amount--;
				}
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			}

			if(window_time()-players[local_player_id].item_disabled<0.3F) {
				players[local_player_id].item_showup = window_time();
				if(players[local_player_id].input.buttons.lmb) {
					players[local_player_id].input.buttons.lmb_start = window_time();
				}
				players[local_player_id].input.buttons.rmb = 0;
			} else {
				if(hud_active->render_localplayer) {
					float tmp2 = players[local_player_id].physics.eye.y;
					players[local_player_id].physics.eye.y = last_cy;
					if(camera_mode==CAMERAMODE_FPS)
						glDepthRange(0.0F,0.05F);
					matrix_select(matrix_projection);
					matrix_push();
					matrix_translate(0.0F,-0.25F,0.0F);
					matrix_upload_p();
					matrix_select(matrix_model);
					#ifdef OPENGL_ES
					if(camera_mode==CAMERAMODE_FPS)
						glx_disable_sphericalfog();
					#endif
					player_render(&players[local_player_id],local_player_id,NULL,1);
					#ifdef OPENGL_ES
					if(camera_mode==CAMERAMODE_FPS)
						glx_enable_sphericalfog();
					#endif
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

			if(!map_isair(camera_x,camera_y,camera_z)) {
				glClear(GL_COLOR_BUFFER_BIT);
			}

			glx_disable_sphericalfog();
			if(settings.smooth_fog)
				glDisable(GL_FOG);
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

void init() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	#ifdef OPENGL_ES
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);
	#else
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	#endif
	glClearDepth(1.0F);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_FOG);

	map_colors = malloc(map_size_x*map_size_y*map_size_z*sizeof(unsigned int));
	CHECK_ALLOCATION_ERROR(map_colors)
	memset(map_colors,(unsigned int)0xFFFFFFFF,map_size_x*map_size_y*map_size_z);

	map_minimap = malloc(map_size_x*map_size_z*sizeof(unsigned char)*4);
	CHECK_ALLOCATION_ERROR(map_minimap)
	//set minimap borders (white on 64x64 chunks, black map border)
	memset(map_minimap,0xCCCCCCFF,map_size_x*map_size_z*sizeof(unsigned char)*4);

	glx_init();

	font_init();
	player_init();
	particle_init();
	network_init();
	ping_init();
	kv6_init();
	texture_init();
	sound_init();
	tracer_init();
	hud_init();
	chunk_init();

	weapon_set();
}

void reshape(struct window_instance* window, int width, int height) {
    font_reset();
	glViewport(0,0,width,height);
	settings.window_width = width;
	settings.window_height = height;
    if(settings.vsync<2)
	   window_swapping(settings.vsync);
    if(settings.vsync>1)
       window_swapping(0);
}

void text_input(struct window_instance* window, unsigned int codepoint) {
	if(chat_input_mode==CHAT_NO_INPUT)
		return;

	int len = strlen(chat[0][0]);
	if(len<128) {
		chat[0][0][len] = codepoint;
		chat[0][0][len+1] = 0;
	}
}

void keys(struct window_instance* window, int key, int scancode, int action, int mods) {
    if(action==WINDOW_PRESS) {
        if(config_key(key)->toggle) {
			if(chat_input_mode==CHAT_NO_INPUT) {
				window_pressed_keys[key] = !window_pressed_keys[key];
			}
		} else {
			window_pressed_keys[key] = 1;
		}
	}
	if(action==WINDOW_RELEASE && !config_key(key)->toggle)
		window_pressed_keys[key] = 0;

	#ifdef USE_GLFW
	if(key==WINDOW_KEY_FULLSCREEN && action==WINDOW_PRESS) { //switch between fullscreen
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		if(!settings.fullscreen) {
			glfwSetWindowMonitor(window->impl,glfwGetPrimaryMonitor(),0,0,mode->width,mode->height,mode->refreshRate);
			settings.fullscreen = 1;
		} else {
			glfwSetWindowMonitor(window->impl,NULL,(mode->width-800)/2,(mode->height-600)/2,800,600,0);
			settings.fullscreen = 0;
		}
	}
	#endif

	if(key==WINDOW_KEY_SCREENSHOT && action==WINDOW_PRESS) { //take screenshot
		time_t pic_time;
		time(&pic_time);
		char pic_name[128];
		sprintf(pic_name,"screenshots/%ld.png",(long)pic_time);

		unsigned char* pic_data = malloc(settings.window_width*settings.window_height*4*2);
		CHECK_ALLOCATION_ERROR(pic_data)
		glReadPixels(0,0,settings.window_width,settings.window_height,GL_RGBA,GL_UNSIGNED_BYTE,pic_data);

		for(int y=0;y<settings.window_height;y++) { //mirror image (top-bottom)
            for(int x=0;x<settings.window_width;x++) {
                pic_data[(x+(settings.window_height-y-1)*settings.window_width)*4+3] = 255;
            }
			memcpy(pic_data+settings.window_width*4*(y+settings.window_height),
				   pic_data+settings.window_width*4*(settings.window_height-y-1),
				   settings.window_width*4);
		}

		lodepng_encode32_file(pic_name,pic_data+settings.window_width*settings.window_height*4,settings.window_width,settings.window_height);
		free(pic_data);

		sprintf(pic_name,"Saved screenshot as screenshots/%ld.png", (long)pic_time);
		chat_add(0,0x0000FF,pic_name);
	}
}

void mouse_click(struct window_instance* window, int button, int action, int mods) {
	if(hud_active->input_mouseclick) {
		double x,y;
		window_mouseloc(&x,&y);
		hud_active->input_mouseclick(x,y,button,action,mods);
	}
}

void mouse(struct window_instance* window, double x, double y) {
	if(hud_active->input_mouselocation)
		hud_active->input_mouselocation(x,y);
}

void mouse_scroll(struct window_instance* window, double xoffset, double yoffset) {
	if(hud_active->input_mousescroll)
		hud_active->input_mousescroll(yoffset);
}

void deinit() {
	ping_deinit();
	if(settings.show_news)
		file_url("https://www.buildandshoot.com/news/");
	if(network_connected)
		network_disconnect();
	window_deinit();
}

void on_error(int i, const char* s) {
	log_fatal("Major error occured: [%i] %s",i,s);
	getchar();
}

int main(int argc, char** argv) {
	settings.opengl14 = 1;
	settings.color_correction = 0;
	settings.multisamples = 0;
	settings.shadow_entities = 0;
	settings.ambient_occlusion = 0;
	settings.render_distance = 128.0F;
	settings.window_width = 800;
	settings.window_height = 600;
	settings.player_arms = 0;
	settings.fullscreen = 0;
	settings.greedy_meshing = 0;
	settings.mouse_sensitivity = MOUSE_SENSITIVITY;
	settings.show_news = 1;
	settings.show_fps = 0;
	settings.volume = 10;
	settings.voxlap_models = 0;
	settings.force_displaylist = 0;
	settings.invert_y = 0;
	settings.smooth_fog = 0;
	strcpy(settings.name,"DEV_CLIENT");

	#ifdef USE_TOUCH
		mkdir("/sdcard/BetterSpades");
	#else
		if(!file_dir_exists("logs"))
			file_dir_create("logs");
		if(!file_dir_exists("cache"))
			file_dir_create("cache");
		if(!file_dir_exists("screenshots"))
			file_dir_create("screenshots");
	#endif

	log_set_level(LOG_INFO);

	time_t t = time(NULL);
	char buf[32];
	strftime(buf,32,"logs/%m-%d-%Y.log",localtime(&t));
	log_set_fp(fopen(buf,"a"));

	srand(t);

	log_info("Game started!");

	config_reload();

	window_init();

	#ifndef OPENGL_ES
	if(glewInit()) {
		log_error("Could not load extended OpenGL functions!");
	}
	#endif

	log_info("Vendor: %s",glGetString(GL_VENDOR));
	log_info("Renderer: %s",glGetString(GL_RENDERER));
	log_info("Version: %s",glGetString(GL_VERSION));

	if(settings.multisamples>0) {
		glEnable(GL_MULTISAMPLE);
		log_info("MSAAx%i on",settings.multisamples);
	}

	while(glGetError()!=GL_NO_ERROR);

	init();
	atexit(deinit);

    if(settings.vsync<2)
	   window_swapping(settings.vsync);
    if(settings.vsync>1)
       window_swapping(0);

	if(argc>1) {
		if(!strcmp(argv[1],"--help")) {
			log_info("Usage: client                     [server browser]");
			log_info("       client -aos://<ip>:<port>  [custom address]");
			exit(0);
		}

		if(!network_connect_string(argv[1]+1)) {
			log_error("Error: Connection failed (use --help for instructions)");
			exit(1);
		} else {
			log_info("Connection to %s successful",argv[1]+1);
			hud_change(&hud_ingame);
		}
	}

	float last_frame_start = 0.0F;
	while(!window_closed()) {
		float dt = window_time()-last_frame_start;
		last_frame_start = window_time();

		display(dt);

		particle_update(dt);
		sound_update();
		network_update();

		window_update();

		if(settings.vsync>1 && (window_time()-last_frame_start)<(1.0F/settings.vsync)) {
			double sleep_s = 1.0F/settings.vsync-(window_time()-last_frame_start);
			struct timespec ts;
			ts.tv_sec = (int)sleep_s;
			ts.tv_nsec = (sleep_s-ts.tv_sec)*1000000000.0;
			nanosleep(&ts,NULL);
		}
		fps = 1.0F/(window_time()-last_frame_start);
	}
}
