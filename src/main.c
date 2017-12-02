#include "common.h"

#define MOUSE_SENSITIVITY 0.002F

struct RENDER_OPTIONS settings;

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

int screen_current = SCREEN_NONE;

float drawScene() {
	if(settings.ambient_occlusion) {
		glShadeModel(GL_SMOOTH);
	} else {
		glShadeModel(GL_FLAT);
	}

	matrix_upload();
	glEnableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	float r = chunk_draw_visible();
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glShadeModel(GL_FLAT);
	particle_render();
	tracer_render();

	map_damaged_voxels_render();

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

	return r;
}

int cmp(const void* a, const void* b) {
	struct Player* aa = (struct Player*)a;
	struct Player* bb = (struct Player*)b;
	return bb->score-aa->score;
}

void glxcheckErrors(char* file, int line) {
	/*int err, b = 0;
	while((err=glGetError())!=GL_NO_ERROR) {
		if(err!=GL_NO_ERROR) {
			b = 1;
		}
		char* s[] = {"INVALID_ENUM","INVALID_VALUE","INVALID_OPERATION","STACK_OVERFLOW","STACK_UNDERFLOW","OUT_OF_MEMORY"};
		printf("GLERROR: %s\n",s[err-0x500]);
	}
	if(b) {
		printf("%s: %i",file,line);
		__asm__("int $3");
	}*/
}

void render_HUD_3D() {
	glDepthRange(0.0F,0.05F);

	matrix_select(matrix_projection);
	matrix_identity();
	matrix_perspective(camera_fov,((float)settings.window_width)/((float)settings.window_height), 0.1F, settings.render_distance+CHUNK_SIZE*4.0F+128.0F);
	matrix_upload_p();
	matrix_select(matrix_view);
	matrix_identity();
	matrix_select(matrix_model);

	if(!network_map_transfer) {
		if(camera_mode==CAMERAMODE_FPS && players[local_player_id].items_show) {
			players[local_player_id].input.buttons.rmb = 0;

			matrix_identity();
			matrix_translate(-2.25F,-1.5F-(players[local_player_id].held_item==TOOL_SPADE)*0.5F,-6.0F);
			matrix_rotate(glfwGetTime()*57.4F,0.0F,1.0F,0.0F);
			matrix_translate((model_spade.xpiv-model_spade.xsiz/2)*0.05F,(model_spade.zpiv-model_spade.zsiz/2)*0.05F,(model_spade.ypiv-model_spade.ysiz/2)*0.05F);
			if(players[local_player_id].held_item==TOOL_SPADE) {
				matrix_scale(1.5F,1.5F,1.5F);
			}
			matrix_upload();
			kv6_render(&model_spade,players[local_player_id].team);

			if(local_player_blocks>0) {
				matrix_identity();
				matrix_translate(-2.25F,-1.5F-(players[local_player_id].held_item==TOOL_BLOCK)*0.5F,-6.0F);
				matrix_translate(1.5F,0.0F,0.0F);
				matrix_rotate(glfwGetTime()*57.4F,0.0F,1.0F,0.0F);
				matrix_translate((model_block.xpiv-model_block.xsiz/2)*0.05F,(model_block.zpiv-model_block.zsiz/2)*0.05F,(model_block.ypiv-model_block.ysiz/2)*0.05F);
				if(players[local_player_id].held_item==TOOL_BLOCK) {
					matrix_scale(1.5F,1.5F,1.5F);
				}
				model_block.red = players[local_player_id].block.red/255.0F;
	            model_block.green = players[local_player_id].block.green/255.0F;
	            model_block.blue = players[local_player_id].block.blue/255.0F;
				matrix_upload();
				kv6_render(&model_block,players[local_player_id].team);
			}

			if(local_player_ammo+local_player_ammo_reserved>0) {
				struct kv6_t* gun;
				switch(players[local_player_id].weapon) {
					default:
					case WEAPON_RIFLE:
						gun = &model_semi;
						break;
					case WEAPON_SMG:
						gun = &model_smg;
						break;
					case WEAPON_SHOTGUN:
						gun = &model_shotgun;
						break;
				}
				matrix_identity();
				matrix_translate(-2.25F,-1.5F-(players[local_player_id].held_item==TOOL_GUN)*0.5F,-6.0F);
				matrix_translate(3.0F,0.0F,0.0F);
				matrix_rotate(glfwGetTime()*57.4F,0.0F,1.0F,0.0F);
				matrix_translate((gun->xpiv-gun->xsiz/2)*0.05F,(gun->zpiv-gun->zsiz/2)*0.05F,(gun->ypiv-gun->ysiz/2)*0.05F);
				if(players[local_player_id].held_item==TOOL_GUN) {
					matrix_scale(1.5F,1.5F,1.5F);
				}
				matrix_upload();
				kv6_render(gun,players[local_player_id].team);
			}

			if(local_player_grenades>0) {
				matrix_identity();
				matrix_translate(-2.25F,-1.5F-(players[local_player_id].held_item==TOOL_GRENADE)*0.5F,-6.0F);
				matrix_translate(4.5F,0.0F,0.0F);
				matrix_rotate(glfwGetTime()*57.4F,0.0F,1.0F,0.0F);
				matrix_translate((model_grenade.xpiv-model_grenade.xsiz/2)*0.05F,(model_grenade.zpiv-model_grenade.zsiz/2)*0.05F,(model_grenade.ypiv-model_grenade.ysiz/2)*0.05F);
				if(players[local_player_id].held_item==TOOL_GRENADE) {
					matrix_scale(1.5F,1.5F,1.5F);
				}
				matrix_upload();
				kv6_render(&model_grenade,players[local_player_id].team);
			}
		}

		if(screen_current==SCREEN_TEAM_SELECT) {
			matrix_identity();
			matrix_translate(-1.4F,-2.0F,-3.0F);
			matrix_rotate(-90.0F+22.5F,0.0F,1.0F,0.0F);
			matrix_upload();
			struct Player p_hud;
			memset(&p_hud,0,sizeof(struct Player));
			p_hud.input.keys.packed = 0;
			p_hud.held_item = TOOL_SPADE;
			p_hud.input.buttons.packed = 0;
			p_hud.physics.eye.x = p_hud.pos.x = 0;
			p_hud.physics.eye.y = p_hud.pos.y = 0;
			p_hud.physics.eye.z = p_hud.pos.z = 0;
			p_hud.physics.velocity.x = 0.0F;
			p_hud.physics.velocity.y = 0.0F;
			p_hud.physics.velocity.z = 0.0F;
			p_hud.orientation.x = 1.0F;
			p_hud.orientation.y = 0.0F;
			p_hud.orientation.z = 0.0F;
			p_hud.alive = 1;

			p_hud.team = TEAM_1;
			player_render(&p_hud,PLAYERS_MAX,NULL,1);
			matrix_identity();
			matrix_translate(1.4F,-2.0F,-3.0F);
			matrix_rotate(-90.0F-22.5F,0.0F,1.0F,0.0F);
			matrix_upload();
			p_hud.team = TEAM_2;
			player_render(&p_hud,PLAYERS_MAX,NULL,1);
		}

		if(screen_current==SCREEN_GUN_SELECT) {
			matrix_identity();
			matrix_translate(-1.5F,-1.25F,-3.25F);
			matrix_rotate(glfwGetTime()*90.0F,0.0F,1.0F,0.0F);
			matrix_translate((model_semi.xpiv-model_semi.xsiz/2.0F)*model_semi.scale,
							(model_semi.zpiv-model_semi.zsiz/2.0F)*model_semi.scale,
							(model_semi.ypiv-model_semi.ysiz/2.0F)*model_semi.scale);
			matrix_upload();
			kv6_render(&model_semi,TEAM_SPECTATOR);

			matrix_identity();
			matrix_translate(0.0F,-1.25F,-3.25F);
			matrix_rotate(glfwGetTime()*90.0F,0.0F,1.0F,0.0F);
			matrix_translate((model_smg.xpiv-model_smg.xsiz/2.0F)*model_smg.scale,
							(model_smg.zpiv-model_smg.zsiz/2.0F)*model_smg.scale,
							(model_smg.ypiv-model_smg.ysiz/2.0F)*model_smg.scale);
			matrix_upload();
			kv6_render(&model_smg,TEAM_SPECTATOR);

			matrix_identity();
			matrix_translate(1.5F,-1.25F,-3.25F);
			matrix_rotate(glfwGetTime()*90.0F,0.0F,1.0F,0.0F);
			matrix_translate((model_shotgun.xpiv-model_shotgun.xsiz/2.0F)*model_shotgun.scale,
							(model_shotgun.zpiv-model_shotgun.zsiz/2.0F)*model_shotgun.scale,
							(model_shotgun.ypiv-model_shotgun.ysiz/2.0F)*model_shotgun.scale);
			matrix_upload();
			kv6_render(&model_shotgun,TEAM_SPECTATOR);
		}



		struct kv6_t* rotating_model = NULL;
		int rotating_model_team = TEAM_SPECTATOR;
		if(gamestate.gamemode_type==GAMEMODE_CTF) {
			switch(players[local_player_id].team) {
				case TEAM_1:
					if(gamestate.gamemode.ctf.team_2_intel && gamestate.gamemode.ctf.team_2_intel_location.held.player_id==local_player_id) {
						rotating_model = &model_intel;
						rotating_model_team = TEAM_2;
					}
					break;
				case TEAM_2:
					if(gamestate.gamemode.ctf.team_1_intel && gamestate.gamemode.ctf.team_1_intel_location.held.player_id==local_player_id) {
						rotating_model = &model_intel;
						rotating_model_team = TEAM_1;
					}
					break;
			}
		}
		if(gamestate.gamemode_type==GAMEMODE_TC) {
			for(int k=0;k<gamestate.gamemode.tc.territory_count;k++) {
				float l = pow(gamestate.gamemode.tc.territory[k].x-players[local_player_id].pos.x,2.0F)
						 +pow((63.0F-gamestate.gamemode.tc.territory[k].z)-players[local_player_id].pos.y,2.0F)
						 +pow(gamestate.gamemode.tc.territory[k].y-players[local_player_id].pos.z,2.0F);
				if(l<=20.0F*20.0F) {
					rotating_model = &model_tent;
					rotating_model_team = gamestate.gamemode.tc.territory[k].team;
					break;
				}
			}
		}
		if(rotating_model) {
			matrix_identity();
			matrix_translate(0.0F,-(rotating_model->zsiz*0.5F+rotating_model->zpiv)*rotating_model->scale,-10.0F);
			matrix_rotate(glfwGetTime()*90.0F,0.0F,1.0F,0.0F);
			matrix_upload();
			glViewport(-settings.window_width*0.4F,settings.window_height*0.2F,settings.window_width,settings.window_height);
			kv6_render(rotating_model,rotating_model_team);
			glViewport(0.0F,0.0F,settings.window_width,settings.window_height);
		}
	}
}

void display(float dt) {
	if(network_map_transfer) {
		glClearColor(0.0F,0.0F,0.0F,1.0F);
	} else {
		glClearColor(fog_color[0],fog_color[1],fog_color[2],fog_color[3]);
	}

	if(settings.opengl14) {
		glFogi(GL_FOG_MODE,GL_LINEAR);
		glFogfv(GL_FOG_COLOR,fog_color);
		glFogf(GL_FOG_START,0.0F);
		glFogf(GL_FOG_END,settings.render_distance);
		glEnable(GL_FOG);
		glEnable(GL_DEPTH_TEST);
		glDepthRange(0.0F,1.0F);
	}

	chunk_update_all();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | (GL_STENCIL_BUFFER_BIT*settings.shadow_entities));

	float last_cy = players[local_player_id].physics.eye.y;
	player_move(&players[local_player_id],dt,local_player_id);

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

		double view[16];
		glGetDoublev(GL_MODELVIEW_MATRIX,view);
		float light[4] = {1.0F,-3.0F,1.0F,0.0F};
		mul_matrix_vector(map_sun,view,light);
		float map_sun_len = sqrt(map_sun[0]*map_sun[0]+map_sun[1]*map_sun[1]+map_sun[2]*map_sun[2]);
		map_sun[0] /= map_sun_len;
		map_sun[1] /= map_sun_len;
		map_sun[2] /= map_sun_len;
	}

	camera_ExtractFrustum();

	float fps = 1.0F/dt;
	//printf("FPS: %0.2f\n",fps);

	if(!network_map_transfer) {

		drawScene();

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
				if((int)pos!=0 && pos[1]>1) {
					players[local_player_id].item_showup = glfwGetTime();
					local_player_blocks = max(local_player_blocks-1,0);

					struct PacketBlockAction blk;
					blk.player_id = local_player_id;
					blk.action_type = ACTION_BUILD;
					blk.x = pos[0];
					blk.y = pos[2];
					blk.z = 63-pos[1];
					network_send(PACKET_BLOCKACTION_ID,&blk,sizeof(blk));
				}
			}
		}

		int* pos;
		switch(players[local_player_id].held_item) {
			case TOOL_BLOCK:
				pos = camera_terrain_pick(0);
				break;
			default:
				pos = NULL;
		}
		if(pos!=NULL && (pow(pos[0]-camera_x,2)+pow(pos[1]-camera_y,2)+pow(pos[2]-camera_z,2))<5*5) {
			matrix_upload();
			glColor3f(1.0F,1.0F,1.0F);
			glLineWidth(1.0F);
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
			glBegin(GL_LINES);
			glVertex3s(pos[0],pos[1],pos[2]);
			glVertex3s(pos[0],pos[1],pos[2]+1);
			glVertex3s(pos[0],pos[1],pos[2]);
			glVertex3s(pos[0]+1,pos[1],pos[2]);
			glVertex3s(pos[0]+1,pos[1],pos[2]+1);
			glVertex3s(pos[0]+1,pos[1],pos[2]);
			glVertex3s(pos[0]+1,pos[1],pos[2]+1);
			glVertex3s(pos[0],pos[1],pos[2]+1);

			glVertex3s(pos[0],pos[1]+1,pos[2]);
			glVertex3s(pos[0],pos[1]+1,pos[2]+1);
			glVertex3s(pos[0],pos[1]+1,pos[2]);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]+1);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]+1);
			glVertex3s(pos[0],pos[1]+1,pos[2]+1);

			glVertex3s(pos[0],pos[1],pos[2]);
			glVertex3s(pos[0],pos[1]+1,pos[2]);
			glVertex3s(pos[0]+1,pos[1],pos[2]);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]);
			glVertex3s(pos[0]+1,pos[1],pos[2]+1);
			glVertex3s(pos[0]+1,pos[1]+1,pos[2]+1);
			glVertex3s(pos[0],pos[1],pos[2]+1);
			glVertex3s(pos[0],pos[1]+1,pos[2]+1);
			glEnd();
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

		player_update(dt);

	}

	if(settings.opengl14) {
		glDisable(GL_FOG);
	}

	render_HUD_3D();

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

	if(network_map_transfer) {
		glColor3f(1.0F,1.0F,1.0F);
		texture_draw(&texture_splash,(settings.window_width-settings.window_height*4.0F/3.0F*0.7F)*0.5F,560*scalef,settings.window_height*4.0F/3.0F*0.7F,settings.window_height*0.7F);

		float p = (compressed_chunk_data_estimate>0)?((float)compressed_chunk_data_offset/(float)compressed_chunk_data_estimate):0.0F;;
		glColor3ub(68,68,68);
		texture_draw(&texture_white,(settings.window_width-440.0F*scalef)/2.0F+440.0F*scalef*p,settings.window_height*0.25F,440.0F*scalef*(1.0F-p),20.0F*scalef);
		glColor3ub(255,255,50);
		texture_draw(&texture_white,(settings.window_width-440.0F*scalef)/2.0F,settings.window_height*0.25F,440.0F*scalef*p,20.0F*scalef);
	} else {

		if(players[local_player_id].held_item==TOOL_GRENADE && local_player_grenades==0) {
			players[local_player_id].held_item--;
		}
		if(players[local_player_id].held_item==TOOL_GUN && local_player_ammo+local_player_ammo_reserved==0) {
			players[local_player_id].held_item--;
		}
		if(players[local_player_id].held_item==TOOL_BLOCK && local_player_blocks==0) {
			players[local_player_id].held_item--;
		}

		if(screen_current==SCREEN_TEAM_SELECT) {
			glColor3f(1.0F,0.0F,0.0F);
			char join_str[48];
			sprintf(join_str,"Press 1 to join %s",gamestate.team_1.name);
			font_centered(settings.window_width/4.0F,61*scalef,18.0F*scalef,join_str);
			sprintf(join_str,"Press 2 to join %s",gamestate.team_2.name);
			font_centered(settings.window_width/4.0F*3.0F,61*scalef,18.0F*scalef,join_str);
			font_centered(settings.window_width/2.0F,61*scalef,18.0F*scalef,"Press 3 to spectate");
			glColor3f(1.0F,1.0F,1.0F);
		}

		if(screen_current==SCREEN_GUN_SELECT) {
			glColor3f(1.0F,0.0F,0.0F);
			font_centered(settings.window_width/4.0F*1.0F,61*scalef,18.0F*scalef,"Press 1 to select");
			font_centered(settings.window_width/4.0F*2.0F,61*scalef,18.0F*scalef,"Press 2 to select");
			font_centered(settings.window_width/4.0F*3.0F,61*scalef,18.0F*scalef,"Press 3 to select");
			glColor3f(1.0F,1.0F,1.0F);
		}

		if(key_map[GLFW_KEY_TAB] || camera_mode==CAMERAMODE_SELECTION) {
			//struct Player* p = malloc(sizeof(players));
			//memcpy(p,players,sizeof(players));
			//qsort(p,PLAYERS_MAX,sizeof(players)/PLAYERS_MAX,cmp);
			struct Player* p = players;

			char score_str[8];
			glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
			switch(gamestate.gamemode_type) {
				case GAMEMODE_CTF:
					sprintf(score_str,"%i-%i",gamestate.gamemode.ctf.team_1_score,gamestate.gamemode.ctf.capture_limit);
					break;
				case GAMEMODE_TC:
				{
					int t = 0;
					for(int k=0;k<gamestate.gamemode.tc.territory_count;k++)
						if(gamestate.gamemode.tc.territory[k].team==TEAM_1)
							t++;
					sprintf(score_str,"%i-%i",t,gamestate.gamemode.tc.territory_count);
					break;
				}
			}
			font_centered(settings.window_width/4.0F,487*scalef,53.0F*scalef,score_str);
			glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
			switch(gamestate.gamemode_type) {
				case GAMEMODE_CTF:
					sprintf(score_str,"%i-%i",gamestate.gamemode.ctf.team_2_score,gamestate.gamemode.ctf.capture_limit);
					break;
				case GAMEMODE_TC:
				{
					int t = 0;
					for(int k=0;k<gamestate.gamemode.tc.territory_count;k++)
						if(gamestate.gamemode.tc.territory[k].team==TEAM_2)
							t++;
					sprintf(score_str,"%i-%i",t,gamestate.gamemode.tc.territory_count);
					break;
				}
			}
			font_centered(settings.window_width/4.0F*3.0F,487*scalef,53.0F*scalef,score_str);
			glColor3f(1.0F,1.0F,1.0F);

			unsigned char cntt[3] = {0};
			for(int k=0;k<PLAYERS_MAX;k++) {
				if(p[k].connected) {
					int mul = 0;
					switch(p[k].team) {
						case TEAM_1:
							mul = 1;
							break;
						case TEAM_2:
							mul = 3;
							break;
						default:
						case TEAM_SPECTATOR:
							mul = 2;
							break;
					}
					char id_str[16];
					sprintf(id_str," #%i",k);
					font_render(settings.window_width/4.0F*mul-font_length(18.0F*scalef,p[k].name),(427-18*cntt[mul-1])*scalef,18.0F*scalef,p[k].name);
					font_render(settings.window_width/4.0F*mul,(427-18*cntt[mul-1])*scalef,18.0F*scalef,id_str);
					sprintf(id_str,"     %i",p[k].score);
					font_render(settings.window_width/4.0F*mul,(427-18*cntt[mul-1])*scalef,18.0F*scalef,id_str);
					cntt[mul-1]++;
				}
			}
		}

		if(camera_mode==CAMERAMODE_BODYVIEW) {
			font_select(FONT_SMALLFNT);
			switch(players[cameracontroller_bodyview_player].team) {
				case TEAM_1:
					glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
					break;
				case TEAM_2:
					glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
					break;
			}
			font_centered(settings.window_width/2.0F,settings.window_height*0.25F,8.0F*scalef,players[cameracontroller_bodyview_player].name);
			font_select(FONT_FIXEDSYS);
			if(glfwGetTime()-local_player_death_time<=local_player_respawn_time) {
				glColor3f(1.0F,0.0F,0.0F);
				int cnt = local_player_respawn_time-(int)(glfwGetTime()-local_player_death_time);
				char coin[16];
				sprintf(coin,"INSERT COIN:%i",cnt);
				font_centered(settings.window_width/2.0F,53.0F*scalef,53.0F*scalef,coin);
				if(local_player_respawn_cnt_last!=cnt) {
					if(cnt<4) {
						sound_create(NULL,SOUND_LOCAL,(cnt==1)?&sound_beep1:&sound_beep2,0.0F,0.0F,0.0F);
					}
					local_player_respawn_cnt_last = cnt;
				}
			}
			glColor3f(1.0F,1.0F,1.0F);
		}

		if(camera_mode==CAMERAMODE_FPS) {

			if(glfwGetTime()-chat_popup_timer<0.4F) {
				glColor3f(1.0F,0.0F,0.0F);
				font_render((settings.window_width-font_length(53.0F*scalef,chat_popup))/2.0F,settings.window_height/2.0F,53.0F*scalef,chat_popup);
			}
			glColor3f(1.0F,1.0F,1.0F);

			if(players[local_player_id].held_item==TOOL_GUN && players[local_player_id].input.buttons.rmb) {
				struct texture* zoom;
				switch(players[local_player_id].weapon) {
					case WEAPON_RIFLE:
						zoom = &texture_zoom_semi;
						break;
					case WEAPON_SMG:
						zoom = &texture_zoom_smg;
						break;
					case WEAPON_SHOTGUN:
						zoom = &texture_zoom_shotgun;
						break;
				}
				float zoom_factor = max(0.25F*(1.0F-((glfwGetTime()-weapon_last_shot)/weapon_delay(players[local_player_id].weapon)))+1.0F,1.0F);
				texture_draw(zoom,(settings.window_width-settings.window_height*4.0F/3.0F*zoom_factor)/2.0F,settings.window_height*(zoom_factor*0.5F+0.5F),settings.window_height*4.0F/3.0F*zoom_factor,settings.window_height*zoom_factor);
			} else {
				texture_draw(&texture_target,(settings.window_width-16)/2.0F,(settings.window_height+16)/2.0F,16,16);
			}

			if(local_player_health<=30) {
				glColor3f(1,0,0);
			} else {
				glColor3f(1,1,1);
			}
			char hp[4];
			sprintf(hp,"%i",local_player_health);
			font_render(settings.window_width/2.0F-font_length(53.0F*scalef,hp),53.0F*scalef,53.0F*scalef,hp);
			texture_draw(&texture_health,settings.window_width/2.0F,44.0F*scalef,32.0F*scalef,32.0F*scalef);


			char item_mini_str[32];
			struct texture* item_mini;
			int off = 0;
			glColor3f(1.0F,1.0F,0.0F);
			switch(players[local_player_id].held_item) {
				default:
				case TOOL_BLOCK:
					off = 64*scalef;
				case TOOL_SPADE:
					item_mini = &texture_block;
					sprintf(item_mini_str,"%i",local_player_blocks);
					break;
				case TOOL_GRENADE:
					item_mini = &texture_grenade;
					sprintf(item_mini_str,"%i",local_player_grenades);
					break;
				case TOOL_GUN:
					sprintf(item_mini_str,"%i-%i",local_player_ammo,local_player_ammo_reserved);
					switch(players[local_player_id].weapon) {
						case WEAPON_RIFLE:
							item_mini = &texture_ammo_semi;
							break;
						case WEAPON_SMG:
							item_mini = &texture_ammo_smg;
							break;
						case WEAPON_SHOTGUN:
							item_mini = &texture_ammo_shotgun;
							break;
					}
					if(local_player_ammo==0) {
						glColor3f(1.0F,0.0F,0.0F);
					}
					break;
			}

			texture_draw(item_mini,settings.window_width-44.0F*scalef-off,44.0F*scalef,32.0F*scalef,32.0F*scalef);
			font_render(settings.window_width-font_length(53.0F*scalef,item_mini_str)-44.0F*scalef-off,53.0F*scalef,53.0F*scalef,item_mini_str);
			glColor3f(1.0F,1.0F,1.0F);

			if(players[local_player_id].held_item==TOOL_BLOCK) {
				for(int y=0;y<8;y++) {
					for(int x=0;x<8;x++) {
						if(texture_block_color(x,y)==players[local_player_id].block.packed) {
							unsigned char g = (((int)(glfwGetTime()*4))&1)*0xFF;
							glColor3ub(g,g,g);
							texture_draw_empty(settings.window_width+(x*8-65)*scalef,(65-y*8)*scalef,8*scalef,8*scalef);
							y = 10; //to break outer loop too
							break;
						}
					}
				}
				glColor3f(1.0F,1.0F,1.0F);

				texture_draw(&texture_color_selection,settings.window_width-64*scalef,64*scalef,64*scalef,64*scalef);
			}
		}

		if(camera_mode!=CAMERAMODE_SELECTION) {
			glColor3f(1.0F,1.0F,1.0F);
			font_select(FONT_SMALLFNT);
			if(chat_input_mode!=CHAT_NO_INPUT) {
				switch(chat_input_mode) {
					case CHAT_ALL_INPUT:
						font_render(11.0F*scalef,settings.window_height*0.15F+20.0F*scalef,8.0F*scalef,"Global:");
						break;
					case CHAT_TEAM_INPUT:
						font_render(11.0F*scalef,settings.window_height*0.15F+20.0F*scalef,8.0F*scalef,"Team:");
						break;
				}
				int l = strlen(chat[0][0]);
				chat[0][0][l] = '_';
				chat[0][0][l+1] = 0;
				font_render(11.0F*scalef,settings.window_height*0.15F+10.0F*scalef,8.0F*scalef,chat[0][0]);
				chat[0][0][l] = 0;
			}
			for(int k=0;k<6;k++) {
				if(glfwGetTime()-chat_timer[0][k+1]<10.0F) {
					glColor3ub(red(chat_color[0][k+1]),green(chat_color[0][k+1]),blue(chat_color[0][k+1]));
					font_render(11.0F*scalef,settings.window_height*0.15F-10.0F*scalef*k,8.0F*scalef,chat[0][k+1]);
				}
			}

			for(int k=0;k<6;k++) {
				if(glfwGetTime()-chat_timer[1][k+1]<10.0F) {
					glColor3ub(red(chat_color[1][k+1]),green(chat_color[1][k+1]),blue(chat_color[1][k+1]));
					font_render(11.0F*scalef,settings.window_height-22.0F*scalef-10.0F*scalef*k,8.0F*scalef,chat[1][k+1]);
				}
			}

			font_select(FONT_FIXEDSYS);
			glColor3f(1.0F,1.0F,1.0F);
		} else {
			texture_draw(&texture_splash,(settings.window_width-240*scalef)*0.5F,599*scalef,240*scalef,180*scalef);
		}

		if(gamestate.gamemode_type==GAMEMODE_TC
			&& gamestate.progressbar.tent<gamestate.gamemode.tc.territory_count
			&& gamestate.gamemode.tc.territory[gamestate.progressbar.tent].team!=gamestate.progressbar.team_capturing) {
			float p = max(min(gamestate.progressbar.progress+0.05F*gamestate.progressbar.rate*(glfwGetTime()-gamestate.progressbar.update),1.0F),0.0F);
			float l = pow(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].x-players[local_player_id].pos.x,2.0F)
					 +pow((63.0F-gamestate.gamemode.tc.territory[gamestate.progressbar.tent].z)-players[local_player_id].pos.y,2.0F)
					 +pow(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].y-players[local_player_id].pos.z,2.0F);
			if(p<1.0F && l<20.0F*20.0F) {
				switch(gamestate.gamemode.tc.territory[gamestate.progressbar.tent].team) {
					case TEAM_1:
						glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
						break;
					case TEAM_2:
						glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
						break;
					default:
						glColor3ub(0,0,0);
				}
				texture_draw(&texture_white,(settings.window_width-440.0F*scalef)/2.0F+440.0F*scalef*p,settings.window_height*0.25F,440.0F*scalef*(1.0F-p),20.0F*scalef);
				switch(gamestate.progressbar.team_capturing) {
					case TEAM_1:
						glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
						break;
					case TEAM_2:
						glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
						break;
					default:
						glColor3ub(0,0,0);
				}
				texture_draw(&texture_white,(settings.window_width-440.0F*scalef)/2.0F,settings.window_height*0.25F,440.0F*scalef*p,20.0F*scalef);
			}
		}

		//draw the minimap
		if(camera_mode!=CAMERAMODE_SELECTION) {
			glPixelStorei(GL_UNPACK_ALIGNMENT,1);
			glColor3f(1.0F,1.0F,1.0F);
			//large
			if(key_map[GLFW_KEY_M]) {
				float minimap_x = (settings.window_width-(map_size_x+1)*scalef)/2.0F;
				float minimap_y = ((600-map_size_z-1)/2.0F+map_size_z+1)*scalef;

				glPixelZoom(scalef,-scalef);
				glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
				glRasterPos2f((settings.window_width-(map_size_x+1)*scalef)/2.0F,settings.window_height-(settings.window_height-(map_size_z+1)*scalef)/2.0F);
				glDrawPixels(map_size_x+1,map_size_z+1,GL_RGB,GL_UNSIGNED_BYTE,map_minimap);

				font_select(FONT_SMALLFNT);
				char c[2] = {0};
				for(int k=0;k<8;k++) {
					c[0] = 'A'+k;
					font_centered(minimap_x+(64*k+32)*scalef,minimap_y+8.0F*scalef,8.0F*scalef,c);
					c[0] = '1'+k;
					font_centered(minimap_x-8*scalef,minimap_y-(64*k+32-4)*scalef,8.0F*scalef,c);
				}
				font_select(FONT_FIXEDSYS);

				for(int k=0;k<TRACER_MAX;k++) {
			        if(tracers[k].used) {
						float ang = -atan2(tracers[k].r.direction.z,tracers[k].r.direction.x)-HALFPI;
						texture_draw_rotated(&texture_tracer,minimap_x+tracers[k].r.origin.x*scalef,minimap_y-tracers[k].r.origin.z*scalef,15*scalef,15*scalef,ang);
			        }
			    }

				if(gamestate.gamemode_type==GAMEMODE_CTF) {
					glColor3f(gamestate.team_1.red*0.94F,gamestate.team_1.green*0.94F,gamestate.team_1.blue*0.94F);
					texture_draw_empty_rotated(minimap_x+gamestate.gamemode.ctf.team_1_base.x*scalef,minimap_y-gamestate.gamemode.ctf.team_1_base.y*scalef,12*scalef,12*scalef,0.0F);
					glColor3f(1.0F,1.0F,1.0F);
					texture_draw_rotated(&texture_medical,minimap_x+gamestate.gamemode.ctf.team_1_base.x*scalef,minimap_y-gamestate.gamemode.ctf.team_1_base.y*scalef,12*scalef,12*scalef,0.0F);
					if(!gamestate.gamemode.ctf.team_1_intel) {
						glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
						texture_draw_rotated(&texture_intel,minimap_x+gamestate.gamemode.ctf.team_1_intel_location.dropped.x*scalef,minimap_y-gamestate.gamemode.ctf.team_1_intel_location.dropped.y*scalef,12*scalef,12*scalef,0.0F);
					}

					glColor3f(gamestate.team_2.red*0.94F,gamestate.team_2.green*0.94F,gamestate.team_2.blue*0.94F);
					texture_draw_empty_rotated(minimap_x+gamestate.gamemode.ctf.team_2_base.x*scalef,minimap_y-gamestate.gamemode.ctf.team_2_base.y*scalef,12*scalef,12*scalef,0.0F);
					glColor3f(1.0F,1.0F,1.0F);
					texture_draw_rotated(&texture_medical,minimap_x+gamestate.gamemode.ctf.team_2_base.x*scalef,minimap_y-gamestate.gamemode.ctf.team_2_base.y*scalef,12*scalef,12*scalef,0.0F);
					if(!gamestate.gamemode.ctf.team_2_intel) {
						glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
						texture_draw_rotated(&texture_intel,minimap_x+gamestate.gamemode.ctf.team_2_intel_location.dropped.x*scalef,minimap_y-gamestate.gamemode.ctf.team_2_intel_location.dropped.y*scalef,12*scalef,12*scalef,0.0F);
					}
				}
				if(gamestate.gamemode_type==GAMEMODE_TC) {
					for(int k=0;k<gamestate.gamemode.tc.territory_count;k++) {
						switch(gamestate.gamemode.tc.territory[k].team) {
							case TEAM_1:
								glColor3f(gamestate.team_1.red*0.94F,gamestate.team_1.green*0.94F,gamestate.team_1.blue*0.94F);
								break;
							case TEAM_2:
								glColor3f(gamestate.team_2.red*0.94F,gamestate.team_2.green*0.94F,gamestate.team_2.blue*0.94F);
								break;
							default:
							case TEAM_SPECTATOR:
								glColor3ub(0,0,0);
						}
						texture_draw_rotated(&texture_command,minimap_x+gamestate.gamemode.tc.territory[k].x*scalef,minimap_y-gamestate.gamemode.tc.territory[k].y*scalef,12*scalef,12*scalef,0.0F);
					}
				}

				for(int k=0;k<PLAYERS_MAX;k++) {
					if(players[k].connected && players[k].alive && k!=local_player_id && players[k].team!=TEAM_SPECTATOR && (players[k].team==players[local_player_id].team || camera_mode==CAMERAMODE_SPECTATOR)) {
						switch(players[k].team) {
							case TEAM_1:
								glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
								break;
							case TEAM_2:
								glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
								break;
						}
						float ang = -atan2(players[k].orientation.z,players[k].orientation.x)-HALFPI;
						texture_draw_rotated(&texture_player,minimap_x+players[k].pos.x*scalef,minimap_y-players[k].pos.z*scalef,12*scalef,12*scalef,ang);
					}
				}

				glColor3f(0.0F,1.0F,1.0F);
				texture_draw_rotated(&texture_player,minimap_x+camera_x*scalef,minimap_y-camera_z*scalef,12*scalef,12*scalef,camera_rot_x+PI);
				glColor3f(1.0F,1.0F,1.0F);
			} else {
				//minimized, top left
				float view_x = min(max(camera_x-64.0F,0.0F),map_size_x+1-128.0F);
				float view_z = min(max(camera_z-64.0F,0.0F),map_size_z+1-128.0F);

				glColor3ub(0,0,0);
				texture_draw_empty(settings.window_width-144*scalef,586*scalef,130*scalef,130*scalef);
				glColor3f(1.0F,1.0F,1.0F);

				glPixelZoom(scalef,-scalef);
				glPixelStorei(GL_UNPACK_ROW_LENGTH,map_size_x+1);
				glRasterPos2f(settings.window_width-143*scalef,585*scalef);
				glDrawPixels(128,128,GL_RGB,GL_UNSIGNED_BYTE,map_minimap+((int)view_x+(int)view_z*(map_size_x+1))*3);

				for(int k=0;k<TRACER_MAX;k++) {
			        if(tracers[k].used) {
						float tracer_x = tracers[k].r.origin.x-view_x;
						float tracer_y = tracers[k].r.origin.z-view_z;
						if(tracer_x>0.0F && tracer_x<128.0F && tracer_y>0.0F && tracer_y<128.0F) {
							float ang = -atan2(tracers[k].r.direction.z,tracers[k].r.direction.x)-HALFPI;
							texture_draw_rotated(&texture_tracer,settings.window_width-143*scalef+tracer_x*scalef,(585-tracer_y)*scalef,15*scalef,15*scalef,ang);
						}
			        }
			    }

				if(gamestate.gamemode_type==GAMEMODE_CTF) {
					float tent1_x = min(max(gamestate.gamemode.ctf.team_1_base.x,view_x),view_x+128.0F)-view_x;
					float tent1_y = min(max(gamestate.gamemode.ctf.team_1_base.y,view_z),view_z+128.0F)-view_z;

					float tent2_x = min(max(gamestate.gamemode.ctf.team_2_base.x,view_x),view_x+128.0F)-view_x;
					float tent2_y = min(max(gamestate.gamemode.ctf.team_2_base.y,view_z),view_z+128.0F)-view_z;

					glColor3f(gamestate.team_1.red*0.94F,gamestate.team_1.green*0.94F,gamestate.team_1.blue*0.94F);
					texture_draw_empty_rotated(settings.window_width-143*scalef+tent1_x*scalef,(585-tent1_y)*scalef,12*scalef,12*scalef,0.0F);
					glColor3f(1.0F,1.0F,1.0F);
					texture_draw_rotated(&texture_medical,settings.window_width-143*scalef+tent1_x*scalef,(585-tent1_y)*scalef,12*scalef,12*scalef,0.0F);
					if(!gamestate.gamemode.ctf.team_1_intel) {
						float intel_x = min(max(gamestate.gamemode.ctf.team_1_intel_location.dropped.x,view_x),view_x+128.0F)-view_x;
						float intel_y = min(max(gamestate.gamemode.ctf.team_1_intel_location.dropped.y,view_z),view_z+128.0F)-view_z;
						glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
						texture_draw_rotated(&texture_intel,settings.window_width-143*scalef+intel_x*scalef,(585-intel_y)*scalef,12*scalef,12*scalef,0.0F);
					}

					glColor3f(gamestate.team_2.red*0.94F,gamestate.team_2.green*0.94F,gamestate.team_2.blue*0.94F);
					texture_draw_empty_rotated(settings.window_width-143*scalef+tent2_x*scalef,(585-tent2_y)*scalef,12*scalef,12*scalef,0.0F);
					glColor3f(1.0F,1.0F,1.0F);
					texture_draw_rotated(&texture_medical,settings.window_width-143*scalef+tent2_x*scalef,(585-tent2_y)*scalef,12*scalef,12*scalef,0.0F);
					if(!gamestate.gamemode.ctf.team_2_intel) {
						float intel_x = min(max(gamestate.gamemode.ctf.team_2_intel_location.dropped.x,view_x),view_x+128.0F)-view_x;
						float intel_y = min(max(gamestate.gamemode.ctf.team_2_intel_location.dropped.y,view_z),view_z+128.0F)-view_z;
						glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
						texture_draw_rotated(&texture_intel,settings.window_width-143*scalef+intel_x*scalef,(585-intel_y)*scalef,12*scalef,12*scalef,0.0F);
					}
				}
				if(gamestate.gamemode_type==GAMEMODE_TC) {
					for(int k=0;k<gamestate.gamemode.tc.territory_count;k++) {
						switch(gamestate.gamemode.tc.territory[k].team) {
							case TEAM_1:
								glColor3f(gamestate.team_1.red*0.94F,gamestate.team_1.green*0.94F,gamestate.team_1.blue*0.94F);
								break;
							case TEAM_2:
								glColor3f(gamestate.team_2.red*0.94F,gamestate.team_2.green*0.94F,gamestate.team_2.blue*0.94F);
								break;
							default:
							case TEAM_SPECTATOR:
								glColor3ub(0,0,0);
						}
						float t_x = min(max(gamestate.gamemode.tc.territory[k].x,view_x),view_x+128.0F)-view_x;
						float t_y = min(max(gamestate.gamemode.tc.territory[k].y,view_z),view_z+128.0F)-view_z;
						texture_draw_rotated(&texture_command,settings.window_width-143*scalef+t_x*scalef,(585-t_y)*scalef,12*scalef,12*scalef,0.0F);
					}
				}

				for(int k=0;k<PLAYERS_MAX;k++) {
					if(players[k].connected && players[k].alive && players[k].team!=TEAM_SPECTATOR && (players[k].team==players[local_player_id].team || camera_mode==CAMERAMODE_SPECTATOR)) {
						if(k==local_player_id) {
							glColor3ub(0,255,255);
						} else {
							switch(players[k].team) {
								case TEAM_1:
									glColor3ub(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
									break;
								case TEAM_2:
									glColor3ub(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
									break;
							}
						}
						float player_x = min(max((k==local_player_id)?camera_x:players[k].pos.x,view_x),view_x+128.0F)-view_x;
						float player_y = min(max((k==local_player_id)?camera_z:players[k].pos.z,view_z),view_z+128.0F)-view_z;
						float ang = (k==local_player_id)?camera_rot_x+PI:-atan2(players[k].orientation.z,players[k].orientation.x)-HALFPI;
						texture_draw_rotated(&texture_player,settings.window_width-143*scalef+player_x*scalef,(585-player_y)*scalef,12*scalef,12*scalef,ang);
					}
				}

			}
		}

		if(player_intersection_type>=0) {
			glColor3f(1.0F,1.0F,1.0F);
			font_select(FONT_SMALLFNT);
			char* th[4] = {"Torso","Head","Arms","Legs"};
			char str[32];
			sprintf(str,"%s's %s",players[player_intersection_player].name,th[player_intersection_type]);
			font_centered(settings.window_width/2.0F,settings.window_height*0.2F,8.0F*scalef,str);
			font_select(FONT_FIXEDSYS);
		}
	}



	if(settings.multisamples>0) {
		glEnable(GL_MULTISAMPLE);
	}

	if(settings.opengl14) {
		glEnable(GL_FOG);
	}
}

void init() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);
	glClearDepth(1.0F);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);

	glxcheckErrors(__FILE__,__LINE__);

	font_init();
	player_init();
	particle_init();
	network_init();
	kv6_init();
	texture_init();
	sound_init();
	tracer_init();

	weapon_set();

	//tent = kv6_load(file_load("FullBody_Arms_3.kv6"));
	//gun = kv6_load(file_load("Rifle_10mm.kv6"));

	map_minimap = malloc((map_size_x+1)*(map_size_z+1)*sizeof(unsigned char)*3);

	//set minimap borders (white on 64x64 chunks, black map border)
	memset(map_minimap,0xCC,(map_size_x+1)*(map_size_z+1)*sizeof(unsigned char)*3);
	for(int k=0;k<map_size_x+1;k++) {
		map_minimap[k*3+0] = map_minimap[k*3+1] = map_minimap[k*3+2] = 0;
		map_minimap[(k+map_size_z*(map_size_x+1))*3+0] = map_minimap[(k+map_size_z*(map_size_x+1))*3+1] = map_minimap[(k+map_size_z*(map_size_x+1))*3+2] = 0;
	}
	for(int k=0;k<map_size_z+1;k++) {
		map_minimap[(k*(map_size_x+1))*3+0] = map_minimap[(k*(map_size_x+1))*3+1] = map_minimap[(k*(map_size_x+1))*3+2] = 0;
		map_minimap[(map_size_x+k*(map_size_x+1))*3+0] = map_minimap[(map_size_x+k*(map_size_x+1))*3+1] = map_minimap[(map_size_x+k*(map_size_x+1))*3+2] = 0;
	}


	map_colors = malloc(map_size_x*map_size_y*map_size_z*8);
	for(int x=0;x<512;x++) { for(int z=0;z<512;z++) { for(int y=0;y<64;y++) {
		map_colors[x+(y*map_size_z+z)*map_size_x] = 0xFFFFFFFF;
	} } }
	//memset(map_colors,0x00000000FFFFFFFF,map_size_x*map_size_y*map_size_z);
	map_vxl_load(file_load("lastsav.vxl"),map_colors);
	chunk_rebuild_all();
}

void reshape(GLFWwindow* window, int width, int height) {
	glViewport(0,0,width,height);
	settings.window_width = width;
	settings.window_height = height;
}

void text_input(GLFWwindow* window, unsigned int codepoint) {
	if(chat_input_mode==CHAT_NO_INPUT) {
		return;
	}

	int len = strlen(chat[0][0]);
	if(len<128) {
		chat[0][0][len] = codepoint;
		chat[0][0][len+1] = 0;
	}
}

float hj,hk,hl;
void keys(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(chat_input_mode==CHAT_NO_INPUT) {
		if(action==GLFW_RELEASE) {
			key_map[key] = 0;
		}
		if(action==GLFW_PRESS) {
			key_map[key] = 1;
			if(key==GLFW_KEY_F1) {
				camera_mode = 0;
			}
			if(key==GLFW_KEY_F2) {
				camera_mode = 1;
			}
			if(key==GLFW_KEY_F3) {
				camera_mode = 2;
			}
			if(key==GLFW_KEY_F4) {
				camera_mode = 3;
			}

			if(key==GLFW_KEY_T) {
				chat_input_mode = CHAT_ALL_INPUT;
				chat[0][0][0] = 0;
			}
			if(key==GLFW_KEY_Y || key==GLFW_KEY_Z) {
				chat_input_mode = CHAT_TEAM_INPUT;
				chat[0][0][0] = 0;
			}

			if((key==GLFW_KEY_UP || key==GLFW_KEY_DOWN || key==GLFW_KEY_LEFT || key==GLFW_KEY_RIGHT) && camera_mode==CAMERAMODE_FPS && players[local_player_id].held_item==TOOL_BLOCK) {
				int y;
				for(y=0;y<8;y++) {
					for(int x=0;x<8;x++) {
						if(texture_block_color(x,y)==players[local_player_id].block.packed) {
							switch(key) {
								case GLFW_KEY_LEFT:
									x = max(x-1,0);
									break;
								case GLFW_KEY_RIGHT:
									x = min(x+1,7);
									break;
								case GLFW_KEY_UP:
									y = max(y-1,0);
									break;
								case GLFW_KEY_DOWN:
									y = min(y+1,7);
									break;
							}
							players[local_player_id].block.packed = texture_block_color(x,y);
							network_updateColor();
							y = 10;
							break;
						}
					}
				}
				if(y<10) {
					players[local_player_id].block.packed = texture_block_color(3,0);
					network_updateColor();
				}
			}

			if(key==GLFW_KEY_R && camera_mode==CAMERAMODE_FPS && players[local_player_id].held_item==TOOL_GUN) {
				weapon_reload();
			}

			if(screen_current==SCREEN_NONE) {
				unsigned char tool_switch = 0;
				switch(key) {
					case GLFW_KEY_1:
						if(players[local_player_id].held_item!=TOOL_SPADE) {
							players[local_player_id].held_item = TOOL_SPADE;
							tool_switch = 1;
						}
						break;
					case GLFW_KEY_2:
						if(players[local_player_id].held_item!=TOOL_BLOCK) {
							players[local_player_id].held_item = TOOL_BLOCK;
							tool_switch = 1;
						}
						break;
					case GLFW_KEY_3:
						if(players[local_player_id].held_item!=TOOL_GUN) {
							players[local_player_id].held_item = TOOL_GUN;
							tool_switch = 1;
						}
						break;
					case GLFW_KEY_4:
						if(players[local_player_id].held_item!=TOOL_GRENADE) {
							players[local_player_id].held_item = TOOL_GRENADE;
							tool_switch = 1;
						}
						break;
				}

				if(tool_switch) {
					sound_create(NULL,SOUND_LOCAL,&sound_switch,0.0F,0.0F,0.0F)->stick_to_player = local_player_id;
					players[local_player_id].item_disabled = glfwGetTime();
			        players[local_player_id].items_show_start = glfwGetTime();
			        players[local_player_id].items_show = 1;
				}

				if(key==GLFW_KEY_COMMA) {
					screen_current = SCREEN_TEAM_SELECT;
					return;
				}
				if(key==GLFW_KEY_PERIOD) {
					screen_current = SCREEN_GUN_SELECT;
					return;
				}
			}
			if(screen_current==SCREEN_TEAM_SELECT) {
				int new_team = 255;
				switch(key) {
					case GLFW_KEY_1:
						new_team = TEAM_1;
						break;
					case GLFW_KEY_2:
						new_team = TEAM_2;
						break;
					case GLFW_KEY_3:
						new_team = TEAM_SPECTATOR;
						break;
				}
				if(new_team<255) {
					if(network_logged_in) {
						struct PacketChangeTeam p;
						p.player_id = local_player_id;
						p.team = new_team;
						network_send(PACKET_CHANGETEAM_ID,&p,sizeof(p));
						screen_current = SCREEN_NONE;
						return;
					} else {
						local_player_newteam = new_team;
						screen_current = SCREEN_GUN_SELECT;
						return;
					}
				}
				if(key==GLFW_KEY_COMMA) {
					screen_current = SCREEN_NONE;
				}
			}
			if(screen_current==SCREEN_GUN_SELECT) {
				int new_gun = 255;
				switch(key) {
					case GLFW_KEY_1:
						new_gun = WEAPON_RIFLE;
						break;
					case GLFW_KEY_2:
						new_gun = WEAPON_SMG;
						break;
					case GLFW_KEY_3:
						new_gun = WEAPON_SHOTGUN;
						break;
				}
				if(new_gun<255) {
					if(network_logged_in) {
						struct PacketChangeWeapon p;
						p.player_id = local_player_id;
						p.weapon = new_gun;
						network_send(PACKET_CHANGEWEAPON_ID,&p,sizeof(p));
					} else {
						struct PacketExistingPlayer login;
						login.player_id = local_player_id;
						login.team = local_player_newteam;
						login.weapon = new_gun;
						login.held_item = TOOL_GUN;
						login.kills = 0;
						login.blue = players[local_player_id].block.blue;
						login.green = players[local_player_id].block.green;
						login.red = players[local_player_id].block.red;
						char* n = "DEV_CLIENT";
						strcpy(login.name,n);
						network_send(PACKET_EXISTINGPLAYER_ID,&login,sizeof(login)-sizeof(login.name)+strlen(n)+1);
					}
					screen_current = SCREEN_NONE;
					return;
				}
				if(key==GLFW_KEY_PERIOD) {
					screen_current = SCREEN_NONE;
				}
			}

			if(key==GLFW_KEY_8) {
				char* addr = "aos://151723184:32888:0.75";
				int ip_start = 1;
				for(;addr[ip_start-1]!='/' && addr[ip_start]!='/' && addr[ip_start];ip_start++);
				int port_start = ip_start;
				for(;addr[port_start+1] && addr[port_start]!=':';port_start++);

				int ip = atoi((char*)(addr+ip_start+2));
				char ip_str[17];
				sprintf(ip_str,"%i.%i.%i.%i",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);
				if(!network_connect(ip_str,atoi((char*)(addr+port_start+1)))) {
					printf("connection failed ;(\n");
				}
			}

			/*if(key==GLFW_KEY_9 && !network_logged_in) {
				struct PacketExistingPlayer login;
				login.player_id = local_player_id;
				login.team = TEAM_1;
				login.weapon = WEAPON_SMG;
				login.held_item = TOOL_GUN;
				login.kills = 0;
				login.blue = players[local_player_id].block.blue;
				login.green = players[local_player_id].block.green;
				login.red = players[local_player_id].block.red;
				char* n = "DEV_CLIENT";
				strcpy(login.name,n);
				network_send(PACKET_EXISTINGPLAYER_ID,&login,sizeof(login)-sizeof(login.name)+strlen(n)+1);
			}*/

			if(key==GLFW_KEY_O) {
				draw_outline = (~draw_outline)&0x01;
				chunk_set_render_mode(draw_outline);
			}
			if(key==GLFW_KEY_C) {
				particle_create(0x00FFFFFF,hj,hk,hl,5.0F,3.0F,64,0.1F,0.25F);
			}
			if(key==GLFW_KEY_V) {
				printf("%f,%f,%f,%f,%f\n",camera_x,camera_y,camera_z,camera_rot_x,camera_rot_y);
				players[local_player_id].pos.x = 256.0F;
				players[local_player_id].pos.y = 50.0F;
				players[local_player_id].pos.z = 256.0F;
				hj = camera_x;
				hk = camera_y;
				hl = camera_z;
			}
			if(key==GLFW_KEY_J) {
				settings.render_distance -= 10.0F;
			}
			if(key==GLFW_KEY_K) {
				settings.render_distance += 10.0F;
			}
			if(key==GLFW_KEY_J || key==GLFW_KEY_K) {
				printf("Changed render distance to: %f blocks\n",settings.render_distance);
			}
			if(key==GLFW_KEY_F) {
				//glutFullScreenToggle();
			}
			if(key==GLFW_KEY_E) {
				int* pos = camera_terrain_pick(2);
				if((int)pos!=0) {
					players[local_player_id].block.packed = map_get(pos[0],pos[1],pos[2]);
					network_updateColor();
				}
			}
		}
	} else {
		if(action!=GLFW_RELEASE) {
			if(key==GLFW_KEY_V && mods&GLFW_MOD_CONTROL) {
				const char* clipboard = glfwGetClipboardString(window);
				if(clipboard)
    				strcpy(chat[0][0]+strlen(chat[0][0]),clipboard);
			}
			if(key==GLFW_KEY_ESCAPE || key==GLFW_KEY_ENTER) {
				if(key==GLFW_KEY_ENTER && strlen(chat[0][0])>0) {
					struct PacketChatMessage msg;
					msg.player_id = local_player_id;
					msg.chat_type = CHAT_ALL_INPUT?CHAT_ALL:CHAT_TEAM;
					strcpy(msg.message,chat[0][0]);
					network_send(PACKET_CHATMESSAGE_ID,&msg,sizeof(msg)-sizeof(msg.message)+strlen(chat[0][0])+1);
				}
				chat_input_mode = CHAT_NO_INPUT;
			}
			if(key==GLFW_KEY_BACKSPACE) {
				chat[0][0][strlen(chat[0][0])-1] = 0;
			}
		}
	}
}

void mouse_click(GLFWwindow* window, int button, int action, int mods) {
	if(button==GLFW_MOUSE_BUTTON_LEFT) {
		button_map[0] = (action==GLFW_PRESS);
	}
	if(button==GLFW_MOUSE_BUTTON_RIGHT) {
		if(action==GLFW_PRESS) {
			players[local_player_id].input.buttons.rmb ^= 1;
			if(players[local_player_id].items_show) {
				players[local_player_id].input.buttons.rmb = 0;
			}
		}
		button_map[1] = (action==GLFW_PRESS);
	}
	if(button==GLFW_MOUSE_BUTTON_MIDDLE) {
		button_map[2] = (action==GLFW_PRESS);
	}
	if(button==GLFW_MOUSE_BUTTON_RIGHT && action==GLFW_PRESS) {
		players[local_player_id].input.buttons.rmb_start = glfwGetTime();
		if(camera_mode==CAMERAMODE_BODYVIEW) {
			cameracontroller_bodyview_player = (cameracontroller_bodyview_player+1)%PLAYERS_MAX;
			while(1) {
		        if(players[cameracontroller_bodyview_player].connected && players[cameracontroller_bodyview_player].team==players[local_player_id].team) {
		            break;
		        }
		        cameracontroller_bodyview_player = (cameracontroller_bodyview_player+1)%PLAYERS_MAX;
		    }
		}
	}
	if(button==GLFW_MOUSE_BUTTON_LEFT) {
		if(camera_mode==CAMERAMODE_FPS && glfwGetTime()-players[local_player_id].item_showup>=0.5F) {
			if(players[local_player_id].held_item==TOOL_GRENADE && local_player_grenades>0) {
				if(action==GLFW_RELEASE) {
					local_player_grenades = max(local_player_grenades-1,0);
					struct PacketGrenade g;
					g.player_id = local_player_id;
					g.fuse_length = max(3.0F-(glfwGetTime()-players[local_player_id].input.buttons.lmb_start),0.0F);
					g.x = players[local_player_id].pos.x;
					g.y = players[local_player_id].pos.z;
					g.z = 63.0F-players[local_player_id].pos.y;
					g.vx = (g.fuse_length==0.0F)?0.0F:players[local_player_id].orientation.x;
					g.vy = (g.fuse_length==0.0F)?0.0F:players[local_player_id].orientation.z;
					g.vz = (g.fuse_length==0.0F)?0.0F:(-players[local_player_id].orientation.y);
					network_send(PACKET_GRENADE_ID,&g,sizeof(g));
					read_PacketGrenade(&g,sizeof(g));
				}
				if(action==GLFW_PRESS) {
					sound_create(NULL,SOUND_LOCAL,&sound_grenade_pin,0.0F,0.0F,0.0F)->stick_to_player = local_player_id;
				}
			}
		}
	}
	if(button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS) {
		players[local_player_id].input.buttons.lmb_start = glfwGetTime();

		if(camera_mode==CAMERAMODE_FPS && glfwGetTime()-players[local_player_id].item_showup>=0.5F) {
			if(players[local_player_id].held_item==TOOL_GUN) {
				if(weapon_reloading()) {
					weapon_reload_abort();
				}
				if(local_player_ammo==0) {
					sound_create(NULL,SOUND_LOCAL,&sound_empty,0.0F,0.0F,0.0F);
					chat_showpopup("RELOAD");
				}
			}
		}

		/*int* pos = camera_terrain_pick(1);
		if((int)pos!=0 && pos[1]>1) {
			unsigned int col = map_get(pos[0],pos[1],pos[2]);
			map_set(pos[0],pos[1],pos[2],0xFFFFFFFF);
			particle_create(col,pos[0]+0.5F,pos[1]+0.5F,pos[2]+0.5F,2.5F,1.0F,16,0.1F,0.25F);
			if((map_get(pos[0],pos[1]+1,pos[2])&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0],pos[1]+1,pos[2])) {
				map_apply_gravity();
			}
			if((map_get(pos[0],pos[1]-1,pos[2])&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0],pos[1]-1,pos[2])) {
				map_apply_gravity();
			}
			if((map_get(pos[0]+1,pos[1],pos[2])&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0]+1,pos[1],pos[2])) {
				map_apply_gravity();
			}
			if((map_get(pos[0]-1,pos[1],pos[2])&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0]-1,pos[1],pos[2])) {
				map_apply_gravity();
			}
			if((map_get(pos[0],pos[1],pos[2]+1)&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0],pos[1],pos[2]+1)) {
				map_apply_gravity();
			}
			if((map_get(pos[0],pos[1],pos[2]-1)&0xFFFFFFFF)!=0xFFFFFFFF && !map_ground_connected(pos[0],pos[1],pos[2]-1)) {
				map_apply_gravity();
			}
		}*/
		if(camera_mode==CAMERAMODE_BODYVIEW) {
			cameracontroller_bodyview_player = (cameracontroller_bodyview_player-1)%PLAYERS_MAX;
			while(1) {
		        if(players[cameracontroller_bodyview_player].connected && players[cameracontroller_bodyview_player].team==players[local_player_id].team) {
		            break;
		        }
		        cameracontroller_bodyview_player = (cameracontroller_bodyview_player-1)%PLAYERS_MAX;
		    }
		}
	}
}

double last_x, last_y;
void mouse(GLFWwindow* window, double x, double y) {
    int dx = x-last_x;
    int dy = y-last_y;
	last_x = x;
	last_y = y;

	float s = 1.0F;
	if(camera_mode==CAMERAMODE_FPS && players[local_player_id].held_item==TOOL_GUN && players[local_player_id].input.buttons.rmb) {
		s = 0.5F;
	}

	camera_rot_x -= dx*MOUSE_SENSITIVITY*s;
	camera_rot_y += dy*MOUSE_SENSITIVITY*s;

	camera_overflow_adjust();
}

void mouse_scroll(GLFWwindow* window, double xoffset, double yoffset) {
	if(camera_mode==CAMERAMODE_FPS && yoffset!=0.0F) {
		if(yoffset>0 && players[local_player_id].held_item==0) {
			players[local_player_id].held_item = 3;
		} else {
			players[local_player_id].held_item += (yoffset<0)?1:-1;
			if(players[local_player_id].held_item>3) {
				players[local_player_id].held_item = 0;
			}
		}
		sound_create(NULL,SOUND_LOCAL,&sound_switch,0.0F,0.0F,0.0F)->stick_to_player = local_player_id;
		players[local_player_id].item_disabled = glfwGetTime();
		players[local_player_id].items_show_start = glfwGetTime();
		players[local_player_id].items_show = 1;
	}
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

	if(argc>2) {
		settings.opengl14 = atoi(argv[1]);
		settings.color_correction = atoi(argv[2]);
		settings.multisamples = atoi(argv[3]);
		settings.shadow_entities = atoi(argv[4]);
		settings.ambient_occlusion = atoi(argv[5]);
	}


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

	//glfwSwapInterval(0); uncomment this to disable vsync

	if(!settings.opengl14) {
		/*#ifdef OS_WINDOWS
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
			glTexImage3D = (PFNGLTEXIMAGE3DPROC)glXGetProcAddress("glTexImage3D");
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

		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);*/
	}

	while(glGetError()!=GL_NO_ERROR);

	init();

	float last_frame;
	while(!glfwWindowShouldClose(window) && !key_map[GLFW_KEY_ESCAPE]) {
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
