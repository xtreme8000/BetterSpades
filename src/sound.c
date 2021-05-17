/*
	Copyright (c) 2017-2020 ByteBit

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

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "sound.h"
#include "config.h"
#include "log.h"
#include "camera.h"
#include "entitysystem.h"

#ifdef USE_SOUND
int sound_enabled = 1;
#else
int sound_enabled = 0;
#endif

struct Sound_source {
	int openal_handle;
	char local;
	int stick_to_player;
};

struct entity_system sound_sources;

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
struct Sound_wav sound_grenade_pin;

struct Sound_wav sound_pickup;
struct Sound_wav sound_horn;

struct Sound_wav sound_rifle_shoot;
struct Sound_wav sound_rifle_reload;
struct Sound_wav sound_smg_shoot;
struct Sound_wav sound_smg_reload;
struct Sound_wav sound_shotgun_shoot;
struct Sound_wav sound_shotgun_reload;
struct Sound_wav sound_shotgun_cock;

struct Sound_wav sound_hitground;
struct Sound_wav sound_hitplayer;
struct Sound_wav sound_build;

struct Sound_wav sound_spade_woosh;
struct Sound_wav sound_spade_whack;

struct Sound_wav sound_death;
struct Sound_wav sound_beep1;
struct Sound_wav sound_beep2;
struct Sound_wav sound_switch;
struct Sound_wav sound_empty;
struct Sound_wav sound_intro;

struct Sound_wav sound_debris;
struct Sound_wav sound_bounce;
struct Sound_wav sound_impact;
struct Sound_wav sound_macros;
struct Sound_wav sound_chat;

void sound_volume(float vol) {
#ifdef USE_SOUND
	if(sound_enabled)
		alListenerf(AL_GAIN, vol);
#endif
}

static void sound_createEx(enum sound_space option, struct Sound_wav* w, float x, float y, float z, float vx, float vy,
						   float vz, int player) {
#ifdef USE_SOUND
	if(!sound_enabled)
		return;

	struct Sound_source s = (struct Sound_source) {
		.local = option == SOUND_LOCAL,
		.stick_to_player = player,
	};

	alGetError();
	alGenSources(1, &s.openal_handle);

	if(alGetError() == AL_NO_ERROR) {
		alSourcef(s.openal_handle, AL_PITCH, 1.0F);
		alSourcef(s.openal_handle, AL_GAIN, 1.0F);
		alSourcef(s.openal_handle, AL_REFERENCE_DISTANCE, s.local ? 0.0F : w->min * SOUND_SCALE);
		alSourcef(s.openal_handle, AL_MAX_DISTANCE, s.local ? 2048.0F : w->max * SOUND_SCALE);
		alSource3f(s.openal_handle, AL_POSITION, s.local ? 0.0F : x * SOUND_SCALE, s.local ? 0.0F : y * SOUND_SCALE,
				   s.local ? 0.0F : z * SOUND_SCALE);
		alSource3f(s.openal_handle, AL_VELOCITY, s.local ? 0.0F : vx * SOUND_SCALE, s.local ? 0.0F : vy * SOUND_SCALE,
				   s.local ? 0.0F : vz * SOUND_SCALE);
		alSourcei(s.openal_handle, AL_SOURCE_RELATIVE, s.local);
		alSourcei(s.openal_handle, AL_LOOPING, AL_FALSE);
		alSourcei(s.openal_handle, AL_BUFFER, w->openal_buffer);

		alSourcePlay(s.openal_handle);

		if(alGetError() == AL_NO_ERROR) {
			entitysys_add(&sound_sources, &s);
		} else {
			alDeleteSources(1, &s.openal_handle);
		}
	}
#endif
}

void sound_create_sticky(struct Sound_wav* w, struct Player* player, int player_id) {
	sound_createEx(SOUND_WORLD, w, player->pos.x, player->pos.y, player->pos.z, 0.0F, 0.0F, 0.0F, player_id);
}

void sound_create(enum sound_space option, struct Sound_wav* w, float x, float y, float z) {
	sound_createEx(option, w, x, y, z, 0.0F, 0.0F, 0.0F, -1);
}

void sound_velocity(struct Sound_source* s, float vx, float vy, float vz) {
#ifdef USE_SOUND
	if(!sound_enabled || s->local)
		return;
	alSource3f(s->openal_handle, AL_VELOCITY, vx * SOUND_SCALE, vy * SOUND_SCALE, vz * SOUND_SCALE);
#endif
}

void sound_position(struct Sound_source* s, float x, float y, float z) {
#ifdef USE_SOUND
	if(!sound_enabled || s->local)
		return;

	alSource3f(s->openal_handle, AL_POSITION, x * SOUND_SCALE, y * SOUND_SCALE, z * SOUND_SCALE);
#endif
}

#ifdef USE_SOUND
static bool sound_update_single(void* obj, void* user) {
	struct Sound_source* s = (struct Sound_source*)obj;

	int source_state;
	alGetSourcei(s->openal_handle, AL_SOURCE_STATE, &source_state);
	if(source_state == AL_STOPPED || (s->stick_to_player >= 0 && !players[s->stick_to_player].connected)) {
		alDeleteSources(1, &s->openal_handle);

		return true;
	} else {
		if(s->stick_to_player >= 0) {
			sound_position(s, players[s->stick_to_player].pos.x, players[s->stick_to_player].pos.y,
						   players[s->stick_to_player].pos.z);
			sound_velocity(s, players[s->stick_to_player].physics.velocity.x,
						   players[s->stick_to_player].physics.velocity.y,
						   players[s->stick_to_player].physics.velocity.z);
		}

		return false;
	}
}
#endif

void sound_update() {
#ifdef USE_SOUND
	if(!sound_enabled)
		return;

	float orientation[] = {
		sin(camera_rot_x) * sin(camera_rot_y),
		cos(camera_rot_y),
		cos(camera_rot_x) * sin(camera_rot_y),
		0.0F,
		1.0F,
		0.0F,
	};

	alListener3f(AL_POSITION, camera_x * SOUND_SCALE, camera_y * SOUND_SCALE, camera_z * SOUND_SCALE);
	alListener3f(AL_VELOCITY, camera_vx * SOUND_SCALE, camera_vy * SOUND_SCALE, camera_vz * SOUND_SCALE);
	alListenerfv(AL_ORIENTATION, orientation);

	entitysys_iterate(&sound_sources, NULL, sound_update_single);
#endif
}

extern short* drwav_open_and_read_file_s16(const char* filename, unsigned int* channels, unsigned int* sampleRate,
										   uint64_t* totalFrameCount);

void sound_load(struct Sound_wav* wav, char* name, float min, float max) {
#ifdef USE_SOUND
	if(!sound_enabled)
		return;
	unsigned int channels, samplerate;
	uint64_t samplecount;
	short* samples = drwav_open_and_read_file_s16(name, &channels, &samplerate, &samplecount);
	if(samples == NULL) {
		log_fatal("Could not load sound %s", name);
		exit(1);
	}

	short* audio;
	if(channels > 1) { // convert stereo to mono
		audio = malloc(samplecount * sizeof(short) / 2);
		CHECK_ALLOCATION_ERROR(audio)
		for(int k = 0; k < samplecount / 2; k++)
			audio[k] = ((int)samples[k * 2] + (int)samples[k * 2 + 1]) / 2; // prevent overflow
		free(samples);
	}

	alGenBuffers(1, &wav->openal_buffer);
	alBufferData(wav->openal_buffer, AL_FORMAT_MONO16, (channels > 1) ? audio : samples,
				 samplecount * sizeof(short) / channels, samplerate);

	wav->min = min;
	wav->max = max;
#endif
}

void sound_init() {
#ifdef USE_SOUND
	entitysys_create(&sound_sources, sizeof(struct Sound_source), 256);

	ALCdevice* device = alcOpenDevice(NULL);

	if(!device) {
		sound_enabled = 0;
		log_warn("Could not open sound device!");
		return;
	}

	ALCcontext* context = alcCreateContext(device, NULL);
	if(!alcMakeContextCurrent(context)) {
		sound_enabled = 0;
		log_warn("Could not enter sound device context!");
		return;
	}

	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

	sound_volume(settings.volume / 10.0F);

	sound_load(&sound_footstep1, "wav/footstep1.wav", 0.1F, 32.0F);
	sound_load(&sound_footstep2, "wav/footstep2.wav", 0.1F, 32.0F);
	sound_load(&sound_footstep3, "wav/footstep3.wav", 0.1F, 32.0F);
	sound_load(&sound_footstep4, "wav/footstep4.wav", 0.1F, 32.0F);

	sound_load(&sound_wade1, "wav/wade1.wav", 0.1F, 32.0F);
	sound_load(&sound_wade2, "wav/wade2.wav", 0.1F, 32.0F);
	sound_load(&sound_wade3, "wav/wade3.wav", 0.1F, 32.0F);
	sound_load(&sound_wade4, "wav/wade4.wav", 0.1F, 32.0F);

	sound_load(&sound_jump, "wav/jump.wav", 0.1F, 32.0F);
	sound_load(&sound_land, "wav/land.wav", 0.1F, 32.0F);
	sound_load(&sound_jump_water, "wav/waterjump.wav", 0.1F, 32.0F);
	sound_load(&sound_land_water, "wav/waterland.wav", 0.1F, 32.0F);

	sound_load(&sound_explode, "wav/explode.wav", 0.1F, 53.0F);
	sound_load(&sound_explode_water, "wav/waterexplode.wav", 0.1F, 53.0F);
	sound_load(&sound_grenade_bounce, "wav/grenadebounce.wav", 0.1F, 48.0F);
	sound_load(&sound_grenade_pin, "wav/pin.wav", 0.1F, 48.0F);

	sound_load(&sound_hurt_fall, "wav/fallhurt.wav", 0.1F, 32.0F);

	sound_load(&sound_pickup, "wav/pickup.wav", 0.1F, 1024.0F);
	sound_load(&sound_horn, "wav/horn.wav", 0.1F, 1024.0F);

	sound_load(&sound_rifle_shoot, "wav/semishoot.wav", 0.1F, 96.0F);
	sound_load(&sound_rifle_reload, "wav/semireload.wav", 0.1F, 16.0F);
	sound_load(&sound_smg_shoot, "wav/smgshoot.wav", 0.1F, 96.0F);
	sound_load(&sound_smg_reload, "wav/smgreload.wav", 0.1F, 16.0F);
	sound_load(&sound_shotgun_shoot, "wav/shotgunshoot.wav", 0.1F, 96.0F);
	sound_load(&sound_shotgun_reload, "wav/shotgunreload.wav", 0.1F, 16.0F);
	sound_load(&sound_shotgun_cock, "wav/cock.wav", 0.1F, 16.0F);

	sound_load(&sound_hitground, "wav/hitground.wav", 0.1F, 32.0F);
	sound_load(&sound_hitplayer, "wav/hitplayer.wav", 0.1F, 32.0F);
	sound_load(&sound_build, "wav/build.wav", 0.1F, 32.0F);

	sound_load(&sound_spade_woosh, "wav/woosh.wav", 0.1F, 32.0F);
	sound_load(&sound_spade_whack, "wav/whack.wav", 0.1F, 32.0F);

	sound_load(&sound_death, "wav/death.wav", 0.1F, 24.0F);
	sound_load(&sound_beep1, "wav/beep1.wav", 0.1F, 1024.0F);
	sound_load(&sound_beep2, "wav/beep2.wav", 0.1F, 1024.0F);
	sound_load(&sound_switch, "wav/switch.wav", 0.1F, 1024.0F);
	sound_load(&sound_empty, "wav/empty.wav", 0.1F, 1024.0F);
	sound_load(&sound_intro, "wav/intro.wav", 0.1F, 1024.0F);

	sound_load(&sound_debris, "wav/debris.wav", 0.1F, 53.0F);
	sound_load(&sound_bounce, "wav/bounce.wav", 0.1F, 32.0F);
	sound_load(&sound_impact, "wav/impact.wav", 0.1F, 53.0F);
	sound_load(&sound_macros, "wav/macros_sound.wav", 0.1F, 1024.0F);
	sound_load(&sound_chat, "wav/chat.wav", 0.1F, 1024.0F);
#endif
}
