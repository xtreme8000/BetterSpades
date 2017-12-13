#define PLAYERS_MAX		256 //just because 32 players are not enough
#define TEAM_1			0
#define TEAM_2			1
#define TEAM_SPECTATOR	255

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
extern unsigned char local_player_respawn_time;
extern float		 local_player_death_time;
extern unsigned char local_player_respawn_cnt_last;
extern unsigned char local_player_newteam;

extern float local_player_last_damage_timer;
extern float local_player_last_damage_x;
extern float local_player_last_damage_y;
extern float local_player_last_damage_z;

extern char local_player_drag_active;
extern int local_player_drag_x;
extern int local_player_drag_y;
extern int local_player_drag_z;

extern int player_intersection_type;
extern int player_intersection_player;
extern float player_intersection_dist;

extern struct Player {
	char name[17];
	struct Position {
		float x,y,z;
	} pos;
	struct Orientation {
		float x,y,z;
	} orientation;
	struct Position gun_pos;
	struct Position casing_dir;
	float gun_shoot_timer;
	float spade_use_timer;
	unsigned char spade_used, spade_use_type;
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
				unsigned char lmb : 1;
				unsigned char rmb : 1;
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

float player_section_height(int section);
int player_damage(int damage_sections);
void player_init(void);
float player_height(struct Player* p);
void player_reposition(struct Player* p);
void player_update(float dt);
int player_render(struct Player* p, int id, Ray* ray, char render);
void player_reset(struct Player* p);
int player_move(struct Player* p, float fsynctics, int id);
int player_uncrouch(struct Player* p);
