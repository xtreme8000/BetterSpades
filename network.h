void network_send(int id, void* data, int len);
void network_updateColor();
void network_disconnect();
int network_connect(char* ip, int port);
void network_update();
int network_status();
void network_init();

#pragma pack(push,1)

#define PACKET_POSITIONDATA_ID 0
struct PacketPositionData {
	float x,y,z;
};

#define PACKET_ORIENTATIONDATA_ID 1
struct PacketOrientationData {
	float x,y,z;
};

struct PacketWorldUpdate {
	float x,y,z;
	float ox,oy,oz;
};

#define PACKET_INPUTDATA_ID 3
struct PacketInputData {
	unsigned char player_id;
	unsigned char keys;
};

#define PACKET_WEAPONINPUT_ID 4
struct PacketWeaponInput {
	unsigned char player_id;
	unsigned char primary : 1;
	unsigned char secondary : 1;
};

struct PacketWeaponReload {
	unsigned char player_id;
	unsigned char ammo;
	unsigned char reserved;
};

struct PacketSetHP {
	unsigned char hp;
	unsigned char type;
	float x,y,z;
};

struct PacketKillAction {
	unsigned char player_id;
	unsigned char killer_id;
	unsigned char kill_type;
	unsigned char respawn_time;
};
#define KILLTYPE_WEAPON			0
#define KILLTYPE_HEADSHOT		1
#define KILLTYPE_MELEE			2
#define KILLTYPE_GRENADE		3
#define KILLTYPE_FALL			4
#define KILLTYPE_TEAMCHANGE		5
#define KILLTYPE_CLASSCHANGE	6

struct PacketRestock {
	unsigned char player_id;
};

#define PACKET_GRENADE_ID 6
struct PacketGrenade {
	unsigned char player_id;
	float fuse_length;
	float x,y,z;
	float vx,vy,vz;
};

struct PacketMapStart {
	unsigned int map_size;
};

struct PacketPlayerLeft {
	unsigned char player_id;
};

#define PACKET_EXISTINGPLAYER_ID 9
struct PacketExistingPlayer {
	unsigned char player_id;
	unsigned char team;
	unsigned char weapon;
	unsigned char held_item;
	unsigned int kills;
	unsigned char blue,green,red;
	char name[17];
};
#define WEAPON_RIFLE	0
#define WEAPON_SMG		1
#define WEAPON_SHOTGUN	2


struct PacketCreatePlayer {
	unsigned char player_id;
	unsigned char weapon;
	unsigned char team;
	float x,y,z;
	char name[17];
};

#define PACKET_BLOCKACTION_ID 13
struct PacketBlockAction {
	unsigned char player_id;
	unsigned char action_type;
	int x,y,z;
};
#define ACTION_BUILD	0
#define ACTION_DESTROY	1
#define ACTION_SPADE	2
#define ACTION_GRENADE	3

struct PacketBlockLine {
	unsigned char player_id;
	int sx,sy,sz;
	int ex,ey,ez;
};

#define PACKET_SETCOLOR_ID 8
struct PacketSetColor {
	unsigned char player_id;
	unsigned char blue,green,red;
};

struct PacketShortPlayerData {
	unsigned char player_id;
	unsigned char team;
	unsigned char weapon;
};

struct PacketSetTool {
	unsigned char player_id;
	unsigned char tool;
};
#define TOOL_SPADE		0
#define TOOL_BLOCK		1
#define TOOL_GUN		2
#define TOOL_GRENADE	3

#define PACKET_CHATMESSAGE_ID 17
struct PacketChatMessage {
	unsigned char player_id;
	unsigned char chat_type;
	char message[255];
};
#define CHAT_ALL	0
#define CHAT_TEAM	1
#define CHAT_SYSTEM	2

struct PacketFogColor {
	unsigned char alpha,red,green,blue;
};

struct PacketChangeWeapon {
	unsigned char player_id;
	unsigned char weapon;
};

struct PacketStateData {
	unsigned char player_id;
	unsigned char fog_blue,fog_green,fog_red;
	unsigned char team_1_blue,team_1_green,team_1_red;
	unsigned char team_2_blue,team_2_green,team_2_red;
	char team_1_name[10];
	char team_2_name[10];
	unsigned char gamemode;

	union Gamemodes {
		struct {
			unsigned char team_1_score;
			unsigned char team_2_score;
			unsigned char capture_limit;
			unsigned char team_1_intel : 1;
			unsigned char team_2_intel : 1;
			union intel_location {
				struct {
					unsigned char player_id;
					unsigned char padding[11];
				} held;
				struct {
					float x,y,z;
				} dropped;
			} team_1_intel_location;
			union intel_location team_2_intel_location;
			struct {
				float x,y,z;
			} team_1_base;
			struct {
				float x,y,z;
			} team_2_base;
		} ctf;

		struct {
			unsigned char territory_count;
			struct {
				float x,y,z;
				unsigned char team;
			} territory[16];
		} tc;
	} gamemode_data;
};

#pragma pack(pop)
