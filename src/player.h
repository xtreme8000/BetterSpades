#define PLAYERS_MAX		256 //just because 32 players are not enough
#define TEAM_1			0
#define TEAM_2			1
#define TEAM_SPECTATOR	2

extern struct GameState {
	struct Team {
		char name[10];
		unsigned char red,green,blue;
	} team_1;
	struct Team team_2;
	unsigned char gamemode_type;
	union Gamemodes gamemode;
	struct {
		unsigned char team_capturing, tent;
		float progress, rate, update;
	} progressbar;
} gamestate;
#define GAMEMODE_CTF	0
#define GAMEMODE_TC		1

extern unsigned char local_player_id;
extern unsigned char local_player_health;
extern unsigned char local_player_blocks;
extern unsigned char local_player_grenades;
extern unsigned char local_player_ammo, local_player_ammo_reserved;

extern struct Player {
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
	float item_showup, item_disabled, items_show_start;
	unsigned char items_show;
	union {
		unsigned int packed;
		struct {
			unsigned char red, green, blue;
		};
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
				unsigned char lmb, rmb;
				float lmb_start, rmb_start;
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

	struct {
		struct Sound_source feet;
		float feet_started;
		struct Sound_source tool;
		float tool_started;
	} sound;
} players[PLAYERS_MAX];
//pyspades/pysnip/piqueserver sometime uses ids that are out of range

void player_render(struct Player* p, int id);
void player_reset(struct Player* p);
int player_move(struct Player* p, float fsynctics);
