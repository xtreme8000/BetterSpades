#include <AL/al.h>
#include <AL/alc.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define SOUND_SCALE         0.6F

#define SOUND_VOICES_MAX    4096

#define SOUND_WORLD         0
#define SOUND_LOCAL         1

unsigned char sound_enabled = 1;

struct Sound_source {
    int openal_handle;
    int active;
    unsigned char local;
};

struct Sound_wav {
    int openal_buffer;
    float min, max;
};

struct Sound_source sound_sources[SOUND_VOICES_MAX];
unsigned char sound_sources_free[SOUND_VOICES_MAX];

struct Sound_wav sound_footstep1;
struct Sound_wav sound_footstep2;
struct Sound_wav sound_footstep3;
struct Sound_wav sound_footstep4;

struct Sound_wav sound_wade1;
struct Sound_wav sound_wade2;
struct Sound_wav sound_wade3;
struct Sound_wav sound_wade4;

struct Sound_wav sound_jump;
struct Sound_wav sound_jump_water;

struct Sound_wav sound_land;
struct Sound_wav sound_land_water;

struct Sound_wav sound_hurt_fall;

struct Sound_wav sound_explode;
struct Sound_wav sound_explode_water;
struct Sound_wav sound_grenade_bounce;

struct Sound_wav sound_pickup;
struct Sound_wav sound_horn;

struct Sound_wav sound_rifle_shoot;
struct Sound_wav sound_rifle_reload;
struct Sound_wav sound_smg_shoot;
struct Sound_wav sound_smg_reload;
struct Sound_wav sound_shotgun_shoot;
struct Sound_wav sound_shotgun_reload;


void sound_create(struct Sound_source* s, int option, struct Sound_wav* w, float x, float y, float z);
void sound_createEx(struct Sound_source* s, int option, struct Sound_wav* w, float x, float y, float z, float vx, float vy, float vz);
void sound_velocity(struct Sound_source* s, float vx, float vy, float vz);
void sound_position(struct Sound_source* s, float x, float y, float z);
void sound_update();
void sound_load(struct Sound_wav* wav, char* name, float min, float max);
void sound_init(void);
