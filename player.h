#define PLAYERS_MAX		64
#define TEAM_1			0
#define TEAM_2			1
#define TEAM_SPECTATOR	2

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
		union {
			unsigned char packed;
			struct {
				unsigned char lmb,rmb;
			};
		} buttons;
	} input;

    struct {
        unsigned char jump, airborne, wade;
        float lastclimb;
        struct Velocity {
            float x,y,z;
        } velocity;
        struct Position eye;
    } physics;
} players[PLAYERS_MAX]; //pyspades/pysnip/piqueserver sometime uses ids that are out of range
