#pragma pack(push,1)

struct PacketPositionData {
	float x,y,z;
};

struct PacketOrientationData {
	float x,y,z;
};

struct PacketWorldUpdate {
	struct { float x,y,z,ox,oy,oz; } players[32];
};

struct PacketInputData {
	unsigned char player_id;
	unsigned char keys;
};

struct PacketWeaponInput {
	unsigned char player_id;
	unsigned char primary : 1;
	unsigned char secondary : 1;
};

struct PacketSetHP {
	unsigned char hp;
	unsigned char type;
	float x,y,z;
};

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

struct PacketExistingPlayer {
	unsigned char player_id;
	unsigned char team;
	unsigned char weapon;
	unsigned char held_item;
	unsigned int kills;
	unsigned char blue,green,red;
	char name[17];
};

struct PacketCreatePlayer {
	unsigned char player_id;
	unsigned char weapon;
	unsigned char team;
	float x,y,z;
	char name[17];
};

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

struct PacketChatMessage {
	unsigned char player_id;
	unsigned char chat_type;
	char message[255];
};
#define CHAT_ALL	0
#define CHAT_TEAM	1
#define CHAT_SYSTEM	2

struct PacketStateData {
	unsigned char player_id;
	unsigned char fog_blue,fog_green,fog_red;
	unsigned char team_1_blue,team_1_green,team_1_red;
	unsigned char team_2_blue,team_2_green,team_2_red;
	char team_1_name[10];
	char team_2_name[10];
	unsigned char gamemode;

	union {
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
			float team_1_base_x,team_1_base_y,team_1_base_z;
			float team_2_base_x,team_2_base_y,team_2_base_z;
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
