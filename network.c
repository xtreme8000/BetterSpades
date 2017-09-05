#include <enet/enet.h>
#include "libdeflate.h"

void (*packets[31]) (void* data, int len) = {NULL};
ENetHost* client;
ENetPeer* peer;
int network_connected = 0;

#define VERSION_075	3
#define VERSION_076	4

void* compressed_chunk_data;
int compressed_chunk_data_size;
int compressed_chunk_data_offset = 0;

unsigned char local_player_id = 0;

struct Player {
	char name[17];
	struct Position {
		float x,y,z;
	} pos;
	struct Orientation {
		float x,y,z;
	} orientation;
	unsigned int score;
	unsigned char team, weapon, held_item;
	unsigned char alive, connected;
	struct {
		unsigned char red, green, blue;
	} block;
	struct {
		union {
			unsigned char packed; //useful to load PacketInput quickly
			struct {
				unsigned char up : 1;
				unsigned char down : 1;
				unsigned char left : 1;
				unsigned char right : 1;
				unsigned char jump : 1;
				unsigned char crouch : 1;
				unsigned char sneak : 1;
				unsigned char sprint : 1;
			};
		} keys;
		unsigned char lmb,rmb;
	} input;
} players[64]; //pyspades/pysnip/piqueserver sometime uses ids that are out of range

void read_PacketMapChunk(void* data, int len) {
	//increase allocated memory if it is not enough to store the next chunk
	if(compressed_chunk_data_offset+len>compressed_chunk_data_size) {
		compressed_chunk_data_size += 1024*1024;
		compressed_chunk_data = realloc(compressed_chunk_data,compressed_chunk_data_size);
	}
	//accept any chunk length for "superior" performance, as pointed out by github/NotAFile
	memcpy(compressed_chunk_data+compressed_chunk_data_offset,data,len);
	compressed_chunk_data_offset += len;
}

void read_PacketChatMessage(void* data, int len) {
	struct PacketChatMessage* p = (struct PacketChatMessage*)(data);
	char* n = "";
	if(p->player_id<64) {
		n = players[p->player_id].name;
	}
	printf("[chat] %s: %s\n",n,p->message);
}

void read_PacketBlockAction(void* data, int len) {
	struct PacketBlockAction* p = (struct PacketBlockAction*)(data);
	switch(p->action_type) {
		case ACTION_DESTROY:
			map_set(p->x,63-p->z,p->y,0xFFFFFFFF);
			map_update_physics(p->x,63-p->z,p->y);
			break;
		case ACTION_GRENADE:
			for(int y=(63-(p->z))-1;y<(63-(p->z))+1;y++) {
				for(int z=(p->y)-1;z<(p->y)+1;z++) {
					for(int x=(p->x)-1;x<(p->x)+1;x++) {
						map_set(x,y,z,0xFFFFFFFF);
						map_update_physics(x,y,z);
					}
				}
			}
			break;
		case ACTION_SPADE:
			map_set(p->x,63-p->z-1,p->y,0xFFFFFFFF);
			map_update_physics(p->x,63-p->z-1,p->y);
			map_set(p->x,63-p->z+0,p->y,0xFFFFFFFF);
			map_update_physics(p->x,63-p->z+0,p->y);
			map_set(p->x,63-p->z+1,p->y,0xFFFFFFFF);
			map_update_physics(p->x,63-p->z+1,p->y);
			break;
		case ACTION_BUILD:
			if(p->player_id<64) {
				map_set(p->x,63-p->z,p->y,
					players[p->player_id].block.red |
					(players[p->player_id].block.green<<8) |
					(players[p->player_id].block.blue<<16));
			}
			break;
	}
}

void read_PacketBlockLine(void* data, int len) {
	struct PacketBlockLine* p = (struct PacketBlockLine*)(data);
	if(p->player_id>=64) {
		return;
	}
	if(p->sx==p->ex && p->sy==p->ey && p->sz==p->ez) {
		map_set(p->sx,63-p->sz,p->sy,
			players[p->player_id].block.red |
			(players[p->player_id].block.green>>8) |
			(players[p->player_id].block.blue>>16));
	} else {
		struct Point blocks[64];
		int len = map_cube_line(p->sx,63-p->sz,p->sy,p->ex,63-p->ez,p->ey,blocks);
		while(len>0) {
			map_set(blocks[len-1].x,blocks[len-1].y,blocks[len-1].z,
				players[p->player_id].block.red |
				(players[p->player_id].block.green<<8) |
				(players[p->player_id].block.blue<<16));
			len--;
		}
	}
}

void read_PacketStateData(void* data, int len) {
	struct PacketStateData* p = (struct PacketStateData*)(data);
	//p->gamemode_data.ctf.team_1_intel_location.dropped.x = 5;
	printf("map data was %i bytes\n",compressed_chunk_data_offset);
	int avail_size = 1024*1024;
	void* decompressed = malloc(avail_size);
	int decompressed_size;
	struct libdeflate_decompressor* d = libdeflate_alloc_decompressor();
	while(1) {
		int r = libdeflate_zlib_decompress(d,compressed_chunk_data,compressed_chunk_data_offset,decompressed,avail_size,&decompressed_size);
		//switch not fancy enough here, breaking out of the loop is not aesthetic
		if(r==LIBDEFLATE_INSUFFICIENT_SPACE) {
			avail_size += 1024*1024;
			decompressed = realloc(decompressed,avail_size);
		}
		if(r==LIBDEFLATE_SUCCESS) {
			map_vxl_load(decompressed,map_colors);
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
}

void read_PacketExistingPlayer(void* data, int len) {
	struct PacketExistingPlayer* p = (struct PacketExistingPlayer*)(data);
	if(p->player_id<64) {
		players[p->player_id].connected = 1;
		players[p->player_id].alive = 1;
		players[p->player_id].team = p->team;
		players[p->player_id].weapon = p->weapon;
		players[p->player_id].held_item = p->held_item;
		players[p->player_id].score = p->kills;
		players[p->player_id].block.red = p->red;
		players[p->player_id].block.green = p->green;
		players[p->player_id].block.blue = p->blue;
		strcpy(players[p->player_id].name,p->name);
		printf("existingplayer name: %s\n",p->name);
	}
}

void read_PacketCreatePlayer(void* data, int len) {
	struct PacketCreatePlayer* p = (struct PacketCreatePlayer*)(data);
	if(p->player_id<64) {
		players[p->player_id].connected = 1;
		players[p->player_id].alive = 1;
		players[p->player_id].team = p->team;
		players[p->player_id].weapon = p->weapon;
		players[p->player_id].pos.x = p->x;
		players[p->player_id].pos.y = 63.0F-p->z;
		players[p->player_id].pos.z = p->y;
		strcpy(players[p->player_id].name,p->name);
	}
}

void read_PacketPlayerLeft(void* data, int len) {
	struct PacketPlayerLeft* p = (struct PacketPlayerLeft*)(data);
	if(p->player_id<64) {
		players[p->player_id].connected = 0;
	}
}

void read_PacketMapStart(void* data, int len) {
	struct PacketMapStart* p = (struct PacketMapStart*)(data);
	//someone fix the wrong map size of 1.5mb
	compressed_chunk_data_size = 1024*1024;
	compressed_chunk_data = malloc(compressed_chunk_data_size);
	compressed_chunk_data_offset = 0;
}

void read_PacketWorldUpdate(void* data, int len) {
	struct PacketWorldUpdate* p = (struct PacketWorldUpdate*)(data);
	for(int k=0;k<32;k++) {
		if(players[k].connected) {
			players[k].pos.x = p->players[k].x;
			players[k].pos.y = 63.0F-p->players[k].z;
			players[k].pos.z = p->players[k].y;
			players[k].orientation.x = p->players[k].ox;
			players[k].orientation.y = -p->players[k].oz;
			players[k].orientation.z = p->players[k].oy;
		}
	}
}

void read_PacketPositionData(void* data, int len) {
	struct PacketPositionData* p = (struct PacketPositionData*)(data);
	players[local_player_id].pos.x = p->x;
	players[local_player_id].pos.y = 63.0F-p->z;
	players[local_player_id].pos.z = p->y;
}

void read_PacketOrientationData(void* data, int len) {
	struct PacketOrientationData* p = (struct PacketOrientationData*)(data);
	players[local_player_id].pos.x = p->x;
	players[local_player_id].pos.y = -p->z;
	players[local_player_id].pos.z = p->y;
}

void read_PacketSetColor(void* data, int len) {
	struct PacketSetColor* p = (struct PacketSetColor*)(data);
	if(p->player_id<64) {
		players[p->player_id].block.red = p->red;
		players[p->player_id].block.green = p->green;
		players[p->player_id].block.blue = p->blue;
	}
}

void read_PacketInputData(void* data, int len) {
	struct PacketInputData* p = (struct PacketInputData*)(data);
	if(p->player_id<64) {
		players[p->player_id].input.keys.packed = p->keys;
	}
}

void read_PacketWeaponInput(void* data, int len) {
	struct PacketWeaponInput* p = (struct PacketWeaponInput*)(data);
	if(p->player_id<64) {
		players[p->player_id].input.lmb = p->primary;
		players[p->player_id].input.rmb = p->secondary;
	}
}

void read_PacketSetTool(void* data, int len) {
	struct PacketSetTool* p = (struct PacketSetTool*)(data);
	if(p->player_id<64) {
		players[p->player_id].held_item = p->tool;
	}
}

void network_disconnect() {
	enet_peer_disconnect(peer,0);
	network_connected = 0;

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

int network_connect(char* ip, int port) {
	if(network_connected) {
		network_disconnect();
	}
	ENetAddress address;
	ENetEvent event;
	enet_address_set_host(&address,ip);
	address.port = port;
	peer = enet_host_connect(client,&address,1,VERSION_075);
	if(peer==NULL) {
		return 0;
	}
	if(enet_host_service(client,&event,5000)>0 && event.type == ENET_EVENT_TYPE_CONNECT) {
		network_connected = 1;
		return 1;
	}
	enet_peer_reset(peer);
	return 0;
}

void network_update() {
	if(network_connected) {
		ENetEvent event;
		while(enet_host_service(client,&event,0)>0) {
			switch(event.type) {
				case ENET_EVENT_TYPE_RECEIVE:
					if(*packets[event.packet->data[0]]!=NULL) {
						(*packets[event.packet->data[0]]) (event.packet->data+1,event.packet->dataLength-1);
					} else {
						//printf("Invalid packet id %i\n",event.packet->data[0]);
					}
					enet_packet_destroy(event.packet);
					break;
				case ENET_EVENT_TYPE_DISCONNECT:
					event.peer->data = NULL;
					network_connected = 0;
					break;
			}
		}
	}
}

int network_status() {
	return network_connected;
}

void network_init() {
	enet_initialize();
	client = enet_host_create(NULL,1,1,0,0);
	enet_host_compress_with_range_coder(client);

	packets[0]  = read_PacketPositionData;
	packets[1]  = read_PacketOrientationData;
	packets[2]  = read_PacketWorldUpdate;
	packets[3]  = read_PacketInputData;
	packets[4]  = read_PacketWeaponInput;


	packets[7]  = read_PacketSetTool;
	packets[8]  = read_PacketSetColor;
	packets[9]  = read_PacketExistingPlayer;


	packets[12] = read_PacketCreatePlayer;
	packets[13] = read_PacketBlockAction;
	packets[14] = read_PacketBlockLine;
	packets[15] = read_PacketStateData;

	packets[17] = read_PacketChatMessage;
	packets[18] = read_PacketMapStart;
	packets[19] = read_PacketMapChunk;
}
