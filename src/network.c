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

void (*packets[35]) (void* data, int len) = {NULL};

int network_connected = 0;
int network_logged_in = 0;
int network_map_transfer = 0;
int network_received_packets = 0;
int network_map_cached = 0;

float network_pos_update = 0.0F;
float network_orient_update = 0.0F;
unsigned char network_keys_last = 0;
unsigned char network_buttons_last = 0;
unsigned char network_tool_last = 255;

void* compressed_chunk_data;
int compressed_chunk_data_size;
int compressed_chunk_data_offset = 0;
int compressed_chunk_data_estimate = 0;

ENetHost* client;
ENetPeer* peer;

static void printJoinMsg(int team, char* name) {
	char* t;
	switch(team) {
		case TEAM_1:
			t = gamestate.team_1.name;
			break;
		case TEAM_2:
			t = gamestate.team_2.name;
			break;
		default:
		case TEAM_SPECTATOR:
			t = "Spectator";
			break;
	}
	char s[64];
	sprintf(s,"%s joined the %s team",name,t);
	chat_add(0,0x0000FF,s);
}

void read_PacketMapChunk(void* data, int len) {
	//increase allocated memory if it is not enough to store the next chunk
	if(compressed_chunk_data_offset+len>compressed_chunk_data_size) {
		compressed_chunk_data_size += 1024*1024;
		compressed_chunk_data = realloc(compressed_chunk_data,compressed_chunk_data_size);
		CHECK_ALLOCATION_ERROR(compressed_chunk_data)
	}
	//accept any chunk length for "superior" performance, as pointed out by github/NotAFile
	memcpy(compressed_chunk_data+compressed_chunk_data_offset,data,len);
	compressed_chunk_data_offset += len;
}

void read_PacketChatMessage(void* data, int len) {
	struct PacketChatMessage* p = (struct PacketChatMessage*)data;
	char n[32] = {0};
	char m[256];
	if(p->chat_type!=CHAT_SYSTEM) {
		if(p->player_id<PLAYERS_MAX && players[p->player_id].connected) {
			switch(players[p->player_id].team) {
				case TEAM_1:
					sprintf(n,"%s (%s)",players[p->player_id].name,gamestate.team_1.name);
					break;
				case TEAM_2:
					sprintf(n,"%s (%s)",players[p->player_id].name,gamestate.team_2.name);
					break;
				case TEAM_SPECTATOR:
					sprintf(n,"%s (Spectator)",players[p->player_id].name);
					break;
			}
			sprintf(m,"%s: %s",n,p->message);
		} else {
			sprintf(m,": %s",p->message);
		}
	}
	if(p->chat_type==CHAT_SYSTEM) {
		strcpy(m,p->message);
	}
	printf("%s\n",m);
	unsigned int color;
	switch(p->chat_type) {
		case CHAT_SYSTEM:
			color = 0x0000FF;
			break;
		case CHAT_TEAM:
			switch(players[p->player_id].connected?players[p->player_id].team:players[local_player_id].team) {
				case TEAM_1:
					color = rgb(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue);
					break;
				case TEAM_2:
					color = rgb(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue);
					break;
			}
			break;
		default:
		case CHAT_ALL:
			color = 0xFFFFFF;
			break;
	}
	chat_add(0,color,m);
}

void read_PacketBlockAction(void* data, int len) {
	struct PacketBlockAction* p = (struct PacketBlockAction*)data;
	switch(p->action_type) {
		case ACTION_DESTROY:
			if((63-p->z)>0) {
				int col = map_get(p->x,63-p->z,p->y);
				map_set(p->x,63-p->z,p->y,0xFFFFFFFF);
				map_update_physics(p->x,63-p->z,p->y);
				particle_create(col,p->x+0.5F,63-p->z+0.5F,p->y+0.5F,2.5F,1.0F,8,0.1F,0.25F);
			}
			break;
		case ACTION_GRENADE:
			for(int y=(63-(p->z))-1;y<=(63-(p->z))+1;y++) {
				for(int z=(p->y)-1;z<=(p->y)+1;z++) {
					for(int x=(p->x)-1;x<=(p->x)+1;x++) {
						if(y>1) {
							map_set(x,y,z,0xFFFFFFFF);
							map_update_physics(x,y,z);
						}
					}
				}
			}
			break;
		case ACTION_SPADE:
			if((63-p->z-1)>1) {
				map_set(p->x,63-p->z-1,p->y,0xFFFFFFFF);
				map_update_physics(p->x,63-p->z-1,p->y);
			}
			if((63-p->z+0)>1) {
				map_set(p->x,63-p->z+0,p->y,0xFFFFFFFF);
				map_update_physics(p->x,63-p->z+0,p->y);
			}
			if((63-p->z+1)>1) {
				map_set(p->x,63-p->z+1,p->y,0xFFFFFFFF);
				map_update_physics(p->x,63-p->z+1,p->y);
			}
			break;
		case ACTION_BUILD:
			if(p->player_id<PLAYERS_MAX) {
				map_set(p->x,63-p->z,p->y,
					players[p->player_id].block.red |
					(players[p->player_id].block.green<<8) |
					(players[p->player_id].block.blue<<16));
				sound_create(NULL,SOUND_WORLD,&sound_build,
							 p->x+0.5F,63-p->z+0.5F,p->y+0.5F
						 	);
			}
			break;
	}
}

void read_PacketBlockLine(void* data, int len) {
	struct PacketBlockLine* p = (struct PacketBlockLine*)data;
	if(p->player_id>=PLAYERS_MAX) {
		return;
	}
	if(p->sx==p->ex && p->sy==p->ey && p->sz==p->ez) {
		map_set(p->sx,63-p->sz,p->sy,
			players[p->player_id].block.red |
			(players[p->player_id].block.green<<8) |
			(players[p->player_id].block.blue<<16));
	} else {
		struct Point blocks[64];
		int len = map_cube_line(p->sx,p->sy,p->sz,p->ex,p->ey,p->ez,blocks);
		while(len>0) {
			map_set(blocks[len-1].x,63-blocks[len-1].z,blocks[len-1].y,
				players[p->player_id].block.red |
				(players[p->player_id].block.green<<8) |
				(players[p->player_id].block.blue<<16));
			len--;
		}
	}
	sound_create(NULL,SOUND_WORLD,&sound_build,(p->sx+p->ex)*0.5F+0.5F,(63-p->sz+63-p->ez)*0.5F+0.5F,(p->sy+p->ey)*0.5F+0.5F);
}

void read_PacketStateData(void* data, int len) {
	struct PacketStateData* p = (struct PacketStateData*)data;

	memcpy(gamestate.team_1.name,p->team_1_name,sizeof(p->team_1_name));
	gamestate.team_1.name[sizeof(p->team_1_name)] = 0;

	gamestate.team_1.red = p->team_1_red;
	gamestate.team_1.green = p->team_1_green;
	gamestate.team_1.blue = p->team_1_blue;

	memcpy(gamestate.team_2.name,p->team_2_name,sizeof(p->team_1_name));
	gamestate.team_2.name[sizeof(p->team_1_name)] = 0;

	gamestate.team_2.red = p->team_2_red;
	gamestate.team_2.green = p->team_2_green;
	gamestate.team_2.blue = p->team_2_blue;

	gamestate.gamemode_type = p->gamemode;

	switch(p->gamemode) {
		case GAMEMODE_CTF:
			memcpy(&gamestate.gamemode,&p->gamemode_data,sizeof(struct GM_CTF));
			break;
		case GAMEMODE_TC:
			memcpy(&gamestate.gamemode,&p->gamemode_data,sizeof(struct GM_TC));
			break;
		default:
			printf("Unknown gamemode!\n");
	}

	sound_create(NULL,SOUND_LOCAL,&sound_intro,0.0F,0.0F,0.0F);

	fog_color[0] = p->fog_red/255.0F;
	fog_color[1] = p->fog_green/255.0F;
	fog_color[2] = p->fog_blue/255.0F;

	texture_gradient_fog((unsigned int*)texture_gradient.pixels);
    texture_create_buffer(&texture_gradient,512,512,texture_gradient.pixels);

	local_player_id = p->player_id;
	local_player_health = 100;
	local_player_blocks = 50;
	local_player_grenades = 3;
	weapon_set();

	players[local_player_id].block.red = 111;
	players[local_player_id].block.green = 111;
	players[local_player_id].block.blue = 111;

	camera_mode = CAMERAMODE_SELECTION;
	screen_current = SCREEN_TEAM_SELECT;
	network_map_transfer = 0;

	printf("map data was %i bytes\n",compressed_chunk_data_offset);
	if(!network_map_cached) {
		int avail_size = 1024*1024;
		void* decompressed = malloc(avail_size);
		CHECK_ALLOCATION_ERROR(decompressed)
		size_t decompressed_size;
		struct libdeflate_decompressor* d = libdeflate_alloc_decompressor();
		while(1) {
			int r = libdeflate_zlib_decompress(d,compressed_chunk_data,compressed_chunk_data_offset,decompressed,avail_size,&decompressed_size);
			//switch not fancy enough here, breaking out of the loop is not aesthetic
			if(r==LIBDEFLATE_INSUFFICIENT_SPACE) {
				avail_size += 1024*1024;
				decompressed = realloc(decompressed,avail_size);
				CHECK_ALLOCATION_ERROR(decompressed)
			}
			if(r==LIBDEFLATE_SUCCESS) {
				map_vxl_load(decompressed,map_colors);
				char filename[128];
				sprintf(filename,"cache/%08X.vxl",libdeflate_crc32(0,decompressed,decompressed_size));
				printf(filename);
				FILE* f = fopen(filename,"wb");
				fwrite(decompressed,1,decompressed_size,f);
				fclose(f);
				chunk_rebuild_all();
				break;
			}
			if(r==LIBDEFLATE_BAD_DATA || r==LIBDEFLATE_SHORT_OUTPUT) {
				break;
			}
		}
		free(decompressed);
		free(compressed_chunk_data);
		libdeflate_free_decompressor(d);
	} else {

	}

	kv6_rebuild_all();
}

void read_PacketFogColor(void* data, int len) {
	struct PacketFogColor* p = (struct PacketFogColor*)data;
	fog_color[0] = p->red/255.0F;
	fog_color[1] = p->green/255.0F;
	fog_color[2] = p->blue/255.0F;
	texture_gradient_fog((unsigned int*)texture_gradient.pixels);
    texture_create_buffer(&texture_gradient,512,512,texture_gradient.pixels);
}

void read_PacketExistingPlayer(void* data, int len) {
	struct PacketExistingPlayer* p = (struct PacketExistingPlayer*)data;
	if(p->player_id<PLAYERS_MAX) {
		if(!players[p->player_id].connected)
			printJoinMsg(p->team,p->name);
		player_reset(&players[p->player_id]);
		players[p->player_id].connected = 1;
		players[p->player_id].alive = 1;
		players[p->player_id].team = p->team;
		players[p->player_id].weapon = p->weapon;
		players[p->player_id].held_item = p->held_item;
		players[p->player_id].score = p->kills;
		players[p->player_id].block.red = p->red;
		players[p->player_id].block.green = p->green;
		players[p->player_id].block.blue = p->blue;
		players[p->player_id].ammo = weapon_ammo(p->weapon);
		strcpy(players[p->player_id].name,p->name);
	}
}

void read_PacketCreatePlayer(void* data, int len) {
	struct PacketCreatePlayer* p = (struct PacketCreatePlayer*)data;
	if(p->player_id<PLAYERS_MAX) {
		if(!players[p->player_id].connected)
			printJoinMsg(p->team,p->name);
		player_reset(&players[p->player_id]);
		players[p->player_id].connected = 1;
		players[p->player_id].alive = 1;
		players[p->player_id].team = p->team;
		players[p->player_id].held_item = TOOL_GUN;
		players[p->player_id].weapon = p->weapon;
		players[p->player_id].pos.x = p->x;
		players[p->player_id].pos.y = 63.0F-p->z;
		players[p->player_id].pos.z = p->y;
		strcpy(players[p->player_id].name,p->name);
		players[p->player_id].orientation.x = (p->team==TEAM_1)?1.0F:-1.0F;
		players[p->player_id].orientation.y = 0.0F;
		players[p->player_id].orientation.z = 0.0F;

		players[p->player_id].block.red = 111;
		players[p->player_id].block.green = 111;
		players[p->player_id].block.blue = 111;
		players[p->player_id].ammo = weapon_ammo(p->weapon);
		if(p->player_id==local_player_id) {
			if(p->team==TEAM_SPECTATOR) {
				camera_x = p->x;
				camera_y = 63.0F-p->z;
				camera_z = p->y;
			}
			camera_mode = (p->team==TEAM_SPECTATOR)?CAMERAMODE_SPECTATOR:CAMERAMODE_FPS;
			camera_rot_x = (p->team==TEAM_1)?0.5F*PI:1.5F*PI;
			camera_rot_y = 0.5F*PI;
			network_logged_in = 1;
			local_player_health = 100;
			local_player_blocks = 50;
			local_player_grenades = 3;
			weapon_set();
		}
	}
}

void read_PacketPlayerLeft(void* data, int len) {
	struct PacketPlayerLeft* p = (struct PacketPlayerLeft*)data;
	if(p->player_id<PLAYERS_MAX) {
		players[p->player_id].connected = 0;
		players[p->player_id].alive = 0;
		char s[32];
		sprintf(s,"%s disconnected",players[p->player_id].name);
		chat_add(0,0x0000FF,s);
	}
}

void read_PacketMapStart(void* data, int len) {
	//ffs someone fix the wrong map size of 1.5mb
	compressed_chunk_data_size = 1024*1024;
	compressed_chunk_data = malloc(compressed_chunk_data_size);
	CHECK_ALLOCATION_ERROR(compressed_chunk_data)
	compressed_chunk_data_offset = 0;
	network_logged_in = 0;
	network_map_transfer = 1;
	network_map_cached = 0;

	if(len==sizeof(struct PacketMapStart075)) {
		struct PacketMapStart075* p = (struct PacketMapStart075*)data;
		compressed_chunk_data_estimate = p->map_size;
	} else {
		struct PacketMapStart076* p = (struct PacketMapStart076*)data;
		compressed_chunk_data_estimate = p->map_size;
		printf("map name: %s\n",p->map_name);
		printf("map crc32: 0x%08X\n",p->crc32);
		char filename[128];
		sprintf(filename,"cache/%02X%02X%02X%02X.vxl",red(p->crc32),green(p->crc32),blue(p->crc32),alpha(p->crc32));
		printf(filename);
		if(file_exists(filename)) {
			network_map_cached = 1;
			map_vxl_load(file_load(filename),map_colors);
			chunk_rebuild_all();
		}

		struct PacketMapCached c;
		c.cached = network_map_cached;
		network_send(PACKET_MAPCACHED_ID,&c,sizeof(c));
	}

	player_init();
	camera_mode = CAMERAMODE_SELECTION;
}

void read_PacketWorldUpdate(void* data, int len) {
	if(len>0) {
		char is_075 = (len%sizeof(struct PacketWorldUpdate075)==0);
		char is_076 = (len%sizeof(struct PacketWorldUpdate076)==0);

		if(is_075) {
			for(int k=0;k<(len/sizeof(struct PacketWorldUpdate075));k++) { //supports up to 256 players
				struct PacketWorldUpdate075* p = (struct PacketWorldUpdate075*)(data+k*sizeof(struct PacketWorldUpdate075));
				if(players[k].connected && players[k].alive && k!=local_player_id) {
					if(distance3D(players[k].pos.x,players[k].pos.y,players[k].pos.z,p->x,63.0F-p->z,p->y)>0.1F*0.1F) {
						players[k].pos.x = p->x;
						players[k].pos.y = 63.0F-p->z;
						players[k].pos.z = p->y;
					}
					players[k].orientation.x = p->ox;
					players[k].orientation.y = -p->oz;
					players[k].orientation.z = p->oy;
				}
			}
		} else {
			if(is_076) {
				for(int k=0;k<(len/sizeof(struct PacketWorldUpdate076));k++) {
					struct PacketWorldUpdate076* p = (struct PacketWorldUpdate076*)(data+k*sizeof(struct PacketWorldUpdate076));
					if(players[p->player_id].connected && players[p->player_id].alive && p->player_id!=local_player_id) {
						if(distance3D(players[k].pos.x,players[k].pos.y,players[k].pos.z,p->x,63.0F-p->z,p->y)>0.1F*0.1F) {
							players[p->player_id].pos.x = p->x;
							players[p->player_id].pos.y = 63.0F-p->z;
							players[p->player_id].pos.z = p->y;
						}
						players[p->player_id].orientation.x = p->ox;
						players[p->player_id].orientation.y = -p->oz;
						players[p->player_id].orientation.z = p->oy;
					}
				}
			}
		}
	}
}

void read_PacketPositionData(void* data, int len) {
	struct PacketPositionData* p = (struct PacketPositionData*)data;
	players[local_player_id].pos.x = p->x;
	players[local_player_id].pos.y = 63.0F-p->z;
	players[local_player_id].pos.z = p->y;
}

void read_PacketOrientationData(void* data, int len) {
	struct PacketOrientationData* p = (struct PacketOrientationData*)data;
	players[local_player_id].pos.x = p->x;
	players[local_player_id].pos.y = -p->z;
	players[local_player_id].pos.z = p->y;
}

void read_PacketSetColor(void* data, int len) {
	struct PacketSetColor* p = (struct PacketSetColor*)data;
	if(p->player_id<PLAYERS_MAX) {
		players[p->player_id].block.red = p->red;
		players[p->player_id].block.green = p->green;
		players[p->player_id].block.blue = p->blue;
	}
}

void read_PacketInputData(void* data, int len) {
	struct PacketInputData* p = (struct PacketInputData*)data;
	if(p->player_id<PLAYERS_MAX) {
		if(p->player_id!=local_player_id)
			players[p->player_id].input.keys.packed = p->keys;
		players[p->player_id].physics.jump = (p->keys&16)>0;
		//players[p->player_id].input.keys.jump = (p->keys&16)>0;
	}
}

void read_PacketWeaponInput(void* data, int len) {
	struct PacketWeaponInput* p = (struct PacketWeaponInput*)data;
	if(p->player_id<PLAYERS_MAX && p->player_id!=local_player_id) {
		players[p->player_id].input.buttons.lmb = p->primary;
		players[p->player_id].input.buttons.rmb = p->secondary;
		if(p->primary)
			players[p->player_id].input.buttons.lmb_start = window_time();
		if(p->secondary)
			players[p->player_id].input.buttons.rmb_start = window_time();
	}
}

void read_PacketSetTool(void* data, int len) {
	struct PacketSetTool* p = (struct PacketSetTool*)data;
	if(p->player_id<PLAYERS_MAX && p->tool<4) {
		players[p->player_id].held_item = p->tool;
	}
}

void read_PacketKillAction(void* data, int len) {
	struct PacketKillAction* p = (struct PacketKillAction*)data;
	if(p->player_id<PLAYERS_MAX && p->killer_id<PLAYERS_MAX && p->kill_type>=0 && p->kill_type<7) {
		if(p->player_id==local_player_id) {
			camera_mode = CAMERAMODE_BODYVIEW;
			cameracontroller_bodyview_player = local_player_id;
			local_player_death_time = window_time();
			local_player_respawn_time = p->respawn_time;
			local_player_respawn_cnt_last = 255;
			sound_create(NULL,SOUND_LOCAL,&sound_death,0.0F,0.0F,0.0F);
		}
		players[p->player_id].alive = 0;
		players[p->player_id].input.keys.packed = 0;
		players[p->player_id].input.buttons.packed = 0;
		if(p->player_id!=p->killer_id) {
			players[p->killer_id].score++;
		}
		char* gun_name[3] = {"Rifle","SMG","Shotgun"};
		char m[256];
		switch(p->kill_type) {
			case KILLTYPE_WEAPON:
				sprintf(m,"%s killed %s (%s)",players[p->killer_id].name,players[p->player_id].name,gun_name[players[p->killer_id].weapon]);
				break;
			case KILLTYPE_HEADSHOT:
				sprintf(m,"%s killed %s (Headshot)",players[p->killer_id].name,players[p->player_id].name);
				break;
			case KILLTYPE_MELEE:
				sprintf(m,"%s killed %s (Spade)",players[p->killer_id].name,players[p->player_id].name);
				break;
			case KILLTYPE_GRENADE:
				sprintf(m,"%s killed %s (Grenade)",players[p->killer_id].name,players[p->player_id].name);
				break;
			case KILLTYPE_FALL:
				sprintf(m,"%s fell to far",players[p->player_id].name);
				break;
			case KILLTYPE_TEAMCHANGE:
				sprintf(m,"%s changed teams",players[p->player_id].name);
				break;
			case KILLTYPE_CLASSCHANGE:
				sprintf(m,"%s changed weapons",players[p->player_id].name);
				break;
		}
		if(p->killer_id==local_player_id || p->player_id==local_player_id) {
			chat_add(1,0x0000FF,m);
		} else {
			switch(players[p->killer_id].team) {
				case TEAM_1:
					chat_add(1,rgb(gamestate.team_1.red,gamestate.team_1.green,gamestate.team_1.blue),m);
					break;
				case TEAM_2:
					chat_add(1,rgb(gamestate.team_2.red,gamestate.team_2.green,gamestate.team_2.blue),m);
					break;
			}
		}
	}
}

void read_PacketShortPlayerData(void* data, int len) {
	//should never be received, but process it anyway
	struct PacketShortPlayerData* p = (struct PacketShortPlayerData*)data;
	printf("Unexpected ShortPlayerDataPacket!!!\n");
	/*if(p->player_id<PLAYERS_MAX) {
		players[p->player_id].team = p->team;
		players[p->player_id].weapon = p->weapon;
	}*/
}

void read_PacketGrenade(void* data, int len) {
	struct PacketGrenade* p = (struct PacketGrenade*)data;
	struct Grenade* g = grenade_add();
	g->owner = p->player_id;
	g->fuse_length = p->fuse_length;
	g->pos.x = p->x;
	g->pos.y = 63.0F-p->z;
	g->pos.z = p->y;
	g->velocity.x = p->vx;
	g->velocity.y = -p->vz;
	g->velocity.z = p->vy;
}

void read_PacketSetHP(void* data, int len) {
	struct PacketSetHP* p = (struct PacketSetHP*)data;
	local_player_health = p->hp;
	if(p->type==DAMAGE_SOURCE_GUN) {
		local_player_last_damage_timer = window_time();
		local_player_last_damage_x = p->x;
		local_player_last_damage_y = 63.0F-p->z;
		local_player_last_damage_z = p->y;
		sound_create(NULL,SOUND_LOCAL,&sound_hitplayer,0.0F,0.0F,0.0F);
	}
}

void read_PacketRestock(void* data, int len) {
	struct PacketRestock* p = (struct PacketRestock*)data;
	local_player_health = 100;
	local_player_blocks = 50;
	local_player_grenades = 3;
	weapon_set();
	sound_create(NULL,SOUND_LOCAL,&sound_switch,0.0F,0.0F,0.0F);
}

void read_PacketChangeWeapon(void* data, int len) {
	struct PacketChangeWeapon* p = (struct PacketChangeWeapon*)data;
	if(p->player_id<PLAYERS_MAX) {
		if(p->player_id==local_player_id) {
			printf("Unexpected ChangeWeaponPacket!!!\n");
			return;
		}
		players[p->player_id].weapon = p->weapon;
	}
}

void read_PacketWeaponReload(void* data, int len) {
	struct PacketWeaponReload* p = (struct PacketWeaponReload*)data;
	if(p->player_id==local_player_id) {
		weapon_set();
		local_player_ammo = p->ammo;
		local_player_ammo_reserved = p->reserved;
	} else {
		sound_create(NULL,SOUND_WORLD,weapon_sound_reload(players[p->player_id].weapon),
					 players[p->player_id].pos.x,players[p->player_id].pos.y,players[p->player_id].pos.z
				 )->stick_to_player = p->player_id;
		players[p->player_id].ammo = p->ammo;
	}
}

void read_PacketMoveObject(void* data, int len) {
	struct PacketMoveObject* p = (struct PacketMoveObject*)data;
	if(gamestate.gamemode_type==GAMEMODE_CTF) {
		switch(p->object_id) {
			case TEAM_1_BASE:
				gamestate.gamemode.ctf.team_1_base.x = p->x;
				gamestate.gamemode.ctf.team_1_base.y = p->y;
				gamestate.gamemode.ctf.team_1_base.z = p->z;
				break;
			case TEAM_2_BASE:
				gamestate.gamemode.ctf.team_2_base.x = p->x;
				gamestate.gamemode.ctf.team_2_base.y = p->y;
				gamestate.gamemode.ctf.team_2_base.z = p->z;
				break;
			case TEAM_1_FLAG:
				gamestate.gamemode.ctf.team_1_intel = 0;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.x = p->x;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.y = p->y;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.z = p->z;
				break;
			case TEAM_2_FLAG:
				gamestate.gamemode.ctf.team_2_intel = 0;
				gamestate.gamemode.ctf.team_2_intel_location.dropped.x = p->x;
				gamestate.gamemode.ctf.team_2_intel_location.dropped.y = p->y;
				gamestate.gamemode.ctf.team_2_intel_location.dropped.z = p->z;
				break;
		}
	}
	if(gamestate.gamemode_type==GAMEMODE_TC && p->object_id<gamestate.gamemode.tc.territory_count) {
		gamestate.gamemode.tc.territory[p->object_id].x = p->x;
		gamestate.gamemode.tc.territory[p->object_id].y = p->y;
		gamestate.gamemode.tc.territory[p->object_id].z = p->z;
		gamestate.gamemode.tc.territory[p->object_id].team = p->team;
	}
}

void read_PacketIntelCapture(void* data, int len) {
	struct PacketIntelCapture* p = (struct PacketIntelCapture*)data;
	if(gamestate.gamemode_type==GAMEMODE_CTF && p->player_id<PLAYERS_MAX) {
		switch(players[p->player_id].team) {
			case TEAM_1:
				gamestate.gamemode.ctf.team_1_score++;
				break;
			case TEAM_2:
				gamestate.gamemode.ctf.team_2_score++;
				break;
		}
		//TODO: play horn.wav, show win message etc.
		sound_create(NULL,SOUND_LOCAL,p->winning?&sound_horn:&sound_pickup,0.0F,0.0F,0.0F);
		players[p->player_id].score += 10;
	}
}

void read_PacketIntelDrop(void* data, int len) {
	struct PacketIntelDrop* p = (struct PacketIntelDrop*)data;
	if(gamestate.gamemode_type==GAMEMODE_CTF && p->player_id<PLAYERS_MAX) {
		switch(players[p->player_id].team) {
			case TEAM_1:
				gamestate.gamemode.ctf.team_2_intel = 0; //drop opposing team's intel
				gamestate.gamemode.ctf.team_2_intel_location.dropped.x = p->x;
				gamestate.gamemode.ctf.team_2_intel_location.dropped.y = p->y;
				gamestate.gamemode.ctf.team_2_intel_location.dropped.z = p->z;
				break;
			case TEAM_2:
				gamestate.gamemode.ctf.team_1_intel = 0;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.x = p->x;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.y = p->y;
				gamestate.gamemode.ctf.team_1_intel_location.dropped.z = p->z;
				break;
		}
	}
}

void read_PacketIntelPickup(void* data, int len) {
	struct PacketIntelPickup* p = (struct PacketIntelPickup*)data;
	if(gamestate.gamemode_type==GAMEMODE_CTF && p->player_id<PLAYERS_MAX) {
		switch(players[p->player_id].team) {
			case TEAM_1:
				gamestate.gamemode.ctf.team_2_intel = 1; //pickup opposing team's intel
				gamestate.gamemode.ctf.team_2_intel_location.held.player_id = p->player_id;
				break;
			case TEAM_2:
				gamestate.gamemode.ctf.team_1_intel = 1;
				gamestate.gamemode.ctf.team_1_intel_location.held.player_id = p->player_id;
				break;
		}
	}
}

void read_PacketTerritoryCapture(void* data, int len) {
	struct PacketTerritoryCapture* p = (struct PacketTerritoryCapture*)data;
	if(gamestate.gamemode_type==GAMEMODE_TC && p->tent<gamestate.gamemode.tc.territory_count) {
		gamestate.gamemode.tc.territory[p->tent].team = p->team;
		sound_create(NULL,SOUND_LOCAL,p->winning?&sound_horn:&sound_pickup,0.0F,0.0F,0.0F);
		char x = (int)(gamestate.gamemode.tc.territory[p->tent].x/64.0F)+'A';
		char y = (int)(gamestate.gamemode.tc.territory[p->tent].y/64.0F)+'0';
		char capture_str[128];
		char* team_n;
		switch(p->team) {
			case TEAM_1:
				team_n = gamestate.team_1.name;
				break;
			case TEAM_2:
				team_n = gamestate.team_2.name;
				break;
		}
		if(team_n) {
			sprintf(capture_str,"%s have captured %c%c",team_n,x,y);
			chat_add(0,0x0000FF,capture_str);
		}
	}
}

void read_PacketProgressBar(void* data, int len) {
	struct PacketProgressBar* p = (struct PacketProgressBar*)data;
	if(gamestate.gamemode_type==GAMEMODE_TC && p->tent<gamestate.gamemode.tc.territory_count) {
		gamestate.progressbar.progress = max(min(p->progress,1.0F),0.0F);
		gamestate.progressbar.rate = p->rate;
		gamestate.progressbar.tent = p->tent;
		gamestate.progressbar.team_capturing = p->team_capturing;
		gamestate.progressbar.update = window_time();
	}
}

void read_PacketHandshakeInit(void* data, int len) {
	network_send(PACKET_HANDSHAKERETURN_ID,data,len);
}

void read_PacketVersionGet(void* data, int len) {
	struct PacketVersionSend ver;
	ver.client = 'B';
	ver.major = BETTERSPADES_MAJOR;
	ver.minor = BETTERSPADES_MINOR;
	ver.revision = BETTERSPADES_PATCH;
	#ifdef OS_WINDOWS
		char* os = "BetterSpades (Windows)";
	#endif
	#ifdef OS_LINUX
		char* os = "BetterSpades (Linux)";
	#endif
	#ifdef OS_APPLE
		char* os = "BetterSpades (Apple)";
	#endif
	strcpy(ver.operatingsystem,os);
	network_send(PACKET_VERSIONSEND_ID,&ver,sizeof(ver)-sizeof(ver.operatingsystem)+strlen(os));
}

void network_updateColor() {
	struct PacketSetColor c;
	c.player_id = local_player_id;
	c.red = players[local_player_id].block.red;
	c.green = players[local_player_id].block.green;
	c.blue = players[local_player_id].block.blue;
	network_send(PACKET_SETCOLOR_ID,&c,sizeof(c));
}

unsigned char* network_send_tmp = 0;
void network_send(int id, void* data, int len) {
	if(network_connected) {
		if(!network_send_tmp) {
			network_send_tmp = malloc(512);
			CHECK_ALLOCATION_ERROR(network_send_tmp)
		}
		network_send_tmp[0] = id;
		memcpy(network_send_tmp+1,data,len);
		enet_peer_send(peer,0,enet_packet_create(network_send_tmp,len+1,ENET_PACKET_FLAG_RELIABLE));
	}
}

unsigned int network_ping() {
	return network_connected?peer->roundTripTime:0;
}

void network_disconnect() {
	if(network_connected) {
		enet_peer_disconnect(peer,0);
		network_connected = 0;
		network_logged_in = 0;

		ENetEvent event;
		while(enet_host_service(client,&event,3000)>0) {
			switch(event.type) {
				case ENET_EVENT_TYPE_RECEIVE:
					enet_packet_destroy(event.packet);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					return;
			}
		}

		enet_peer_reset(peer);
	}
}

int network_connect_sub(char* ip, int port, int version) {
	ENetAddress address;
	ENetEvent event;
	enet_address_set_host(&address,ip);
	address.port = port;
	peer = enet_host_connect(client,&address,1,version);
	network_logged_in = 0;
	if(peer==NULL) {
		return 0;
	}
	if(enet_host_service(client,&event,2500)>0 && event.type==ENET_EVENT_TYPE_CONNECT) {;
		network_received_packets = 0;
		network_connected = 1;

		float start = window_time();
		while(window_time()-start<1.0F) { //listen connection for 1s, check if server disconnects
			if(!network_update()) {
				enet_peer_reset(peer);
				return 0;
			}
		}
		return 1;
	}
	enet_peer_reset(peer);
	return 0;
}

int network_connect(char* ip, int port) {
	if(network_connected) {
		network_disconnect();
	}
	if(network_connect_sub(ip,port,VERSION_075)) {
		return 1;
	}
	if(network_connect_sub(ip,port,VERSION_076)) {
		return 1;
	}
	network_connected = 0;
	return 0;
}

int network_connect_string(char* addr) {
	int ip_start = 1;
	for(;addr[ip_start-1]!='/' && addr[ip_start]!='/' && addr[ip_start];ip_start++);
	int port_start = ip_start;
	for(;addr[port_start+1] && addr[port_start]!=':';port_start++);

	int ip = atoi((char*)(addr+ip_start+2));
	char ipport_str[64];
	sprintf(ipport_str,"%i.%i.%i.%i",ip&255,(ip>>8)&255,(ip>>16)&255,(ip>>24)&255);

	return network_connect(ipport_str,atoi((char*)(addr+port_start+1)));
}

int network_update() {
	if(network_connected) {
		ENetEvent event;
		while(enet_host_service(client,&event,0)>0) {
			switch(event.type) {
				case ENET_EVENT_TYPE_RECEIVE:
				{
					int id = event.packet->data[0];
					if(id<sizeof(packets) && *packets[id]!=NULL) {
						//printf("packet id %i\n",id);
						(*packets[id]) (event.packet->data+1,event.packet->dataLength-1);
					} else {
						printf("Invalid packet id %i, length: %i\n",id,(int)event.packet->dataLength-1);
					}
					network_received_packets++;
					enet_packet_destroy(event.packet);
					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
					chat_showpopup("DISCONNECTED");
					printf("server disconnected!\n");
					event.peer->data = NULL;
					network_connected = 0;
					network_logged_in = 0;
					return 0;
			}
		}

		if(network_logged_in && players[local_player_id].team!=TEAM_SPECTATOR) {
			if(players[local_player_id].input.keys.packed!=network_keys_last) {
				struct PacketInputData in;
				in.player_id = local_player_id;
				in.keys = players[local_player_id].input.keys.packed;
				network_send(PACKET_INPUTDATA_ID,&in,sizeof(in));

				network_keys_last = players[local_player_id].input.keys.packed;
			}
			if(players[local_player_id].input.buttons.packed!=network_buttons_last) {
				struct PacketWeaponInput in;
				in.player_id = local_player_id;
				in.primary = players[local_player_id].input.buttons.lmb;
				in.secondary = players[local_player_id].input.buttons.rmb;
				network_send(PACKET_WEAPONINPUT_ID,&in,sizeof(in));

				network_buttons_last = players[local_player_id].input.buttons.packed;
			}
			if(players[local_player_id].held_item!=network_tool_last) {
				struct PacketSetTool t;
				t.player_id = local_player_id;
				t.tool = players[local_player_id].held_item;
				network_send(PACKET_SETTOOL_ID,&t,sizeof(t));

				network_tool_last = players[local_player_id].held_item;
			}

			if(window_time()-network_pos_update>1.0F) {
				network_pos_update = window_time();
				struct PacketPositionData pos;
				pos.x = players[local_player_id].pos.x;
				pos.y = players[local_player_id].pos.z;
				pos.z = 63.0F-players[local_player_id].pos.y;
				network_send(PACKET_POSITIONDATA_ID,&pos,sizeof(pos));
			}
			if(window_time()-network_orient_update>0.05F) {
				network_orient_update = window_time();
				struct PacketPositionData orient;
				orient.x = players[local_player_id].orientation.x;
				orient.y = players[local_player_id].orientation.z;
				orient.z = -players[local_player_id].orientation.y;
				network_send(PACKET_ORIENTATIONDATA_ID,&orient,sizeof(orient));
			}
		}
	}
	return 1;
}

int network_status() {
	return network_connected;
}

void network_init() {
	enet_initialize();
	client = enet_host_create(NULL,1,1,0,0); //limit bandwidth here if you want to
	enet_host_compress_with_range_coder(client);

	packets[PACKET_POSITIONDATA_ID]		= read_PacketPositionData;
	packets[PACKET_ORIENTATIONDATA_ID]	= read_PacketOrientationData;
	packets[PACKET_WORLDUPDATE_ID]		= read_PacketWorldUpdate;
	packets[PACKET_INPUTDATA_ID]		= read_PacketInputData;
	packets[PACKET_WEAPONINPUT_ID]		= read_PacketWeaponInput;
	packets[PACKET_SETHP_ID]			= read_PacketSetHP;
	packets[PACKET_GRENADE_ID]			= read_PacketGrenade;
	packets[PACKET_SETTOOL_ID]			= read_PacketSetTool;
	packets[PACKET_SETCOLOR_ID]			= read_PacketSetColor;
	packets[PACKET_EXISTINGPLAYER_ID]	= read_PacketExistingPlayer;
	packets[PACKET_SHORTPLAYERDATA_ID]	= read_PacketShortPlayerData;
	packets[PACKET_MOVEOBJECT_ID]		= read_PacketMoveObject;
	packets[PACKET_CREATEPLAYER_ID]		= read_PacketCreatePlayer;
	packets[PACKET_BLOCKACTION_ID]		= read_PacketBlockAction;
	packets[PACKET_BLOCKLINE_ID]		= read_PacketBlockLine;
	packets[PACKET_STATEDATA_ID]		= read_PacketStateData;
	packets[PACKET_KILLACTION_ID]		= read_PacketKillAction;
	packets[PACKET_CHATMESSAGE_ID]		= read_PacketChatMessage;
	packets[PACKET_MAPSTART_ID]			= read_PacketMapStart;
	packets[PACKET_MAPCHUNK_ID]			= read_PacketMapChunk;
	packets[PACKET_PLAYERLEFT_ID]		= read_PacketPlayerLeft;
	packets[PACKET_TERRITORYCAPTURE_ID]	= read_PacketTerritoryCapture;
	packets[PACKET_PROGRESSBAR_ID]		= read_PacketProgressBar;
	packets[PACKET_INTELCAPTURE_ID]		= read_PacketIntelCapture;
	packets[PACKET_INTELPICKUP_ID]		= read_PacketIntelPickup;
	packets[PACKET_INTELDROP_ID]		= read_PacketIntelDrop;
	packets[PACKET_RESTOCK_ID]			= read_PacketRestock;
	packets[PACKET_FOGCOLOR_ID]			= read_PacketFogColor;
	packets[PACKET_WEAPONRELOAD_ID]		= read_PacketWeaponReload;
	//29
	packets[PACKET_CHANGEWEAPON_ID]		= read_PacketChangeWeapon;
	packets[PACKET_HANDSHAKEINIT_ID]	= read_PacketHandshakeInit;
	packets[PACKET_VERSIONGET_ID]		= read_PacketVersionGet;
}
