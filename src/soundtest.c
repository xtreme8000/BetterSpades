#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "soundtest.h"
#include "log.h"
#include "sound.h"

void soundtest_next_sound(struct soundtest_ctx * ctx)
{
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	if (ctx->current_sound+1 == ctx->n_sounds)
		return;
	else
		ctx->current_sound++;
#endif
#endif
}

void soundtest_prev_sound(struct soundtest_ctx * ctx)
{
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	if (ctx->current_sound-1 < 0)
		return;
	else
		ctx->current_sound--;
#endif
#endif
}

void soundtest_init(struct soundtest_ctx * ctx)
{
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	log_info("Initializing soundtest context.");
	ctx->current_sound = 0;
	ctx->n_sounds = 40;
	ctx->sounds = malloc(sizeof(struct Sound_wav *) * ctx->n_sounds);
	if (ctx->sounds == NULL) {
		log_fatal("Could not allocate memory for 'ctx->sounds'.");
		exit(1);
	}

	ctx->sound_names = malloc(sizeof(char *) * ctx->n_sounds);
	if (ctx->sound_names == NULL) {
		log_fatal("Could not allocate memory for 'ctx->sound_names'.");
		exit(1);
	}

	for (int i = 0; i < ctx->n_sounds; i++) {
		ctx->sound_names[i] = malloc(sizeof(char) * 64);
		if (ctx->sound_names[i] == NULL) {
			log_fatal("Could not allocate memory for sound name with index %d", i);
			exit(1);
		}
	}
	
	// Initialize sound pointers
	ctx->sounds[0] = &sound_footstep1;
	ctx->sounds[1] = &sound_footstep2;
	ctx->sounds[2] = &sound_footstep3;
	ctx->sounds[3] = &sound_footstep4;

	ctx->sounds[4] = &sound_wade1;
	ctx->sounds[5] = &sound_wade2;
	ctx->sounds[6] = &sound_wade3;
	ctx->sounds[7] = &sound_wade4;

	ctx->sounds[8] = &sound_jump;
	ctx->sounds[9] = &sound_jump_water;

	ctx->sounds[10] = &sound_land;
	ctx->sounds[11] = &sound_land_water;

	ctx->sounds[12] = &sound_hurt_fall;

	ctx->sounds[13] = &sound_explode;
	ctx->sounds[14] = &sound_explode_water;
	ctx->sounds[15] = &sound_grenade_bounce;
	ctx->sounds[16] = &sound_grenade_pin;

	ctx->sounds[17] = &sound_pickup;
	ctx->sounds[18] = &sound_horn;

	ctx->sounds[19] = &sound_rifle_shoot;
	ctx->sounds[20] = &sound_rifle_reload;
	ctx->sounds[21] = &sound_smg_shoot;
	ctx->sounds[22] = &sound_smg_reload;
	ctx->sounds[23] = &sound_shotgun_shoot;
	ctx->sounds[24] = &sound_shotgun_reload;
	ctx->sounds[25] = &sound_shotgun_cock;

	ctx->sounds[26] = &sound_hitground;
	ctx->sounds[27] = &sound_hitplayer;
	ctx->sounds[28] = &sound_build;

	ctx->sounds[29] = &sound_spade_woosh;
	ctx->sounds[30] = &sound_spade_whack;

	ctx->sounds[31] = &sound_death;
	ctx->sounds[32] = &sound_beep1;
	ctx->sounds[33] = &sound_beep2;
	ctx->sounds[34] = &sound_switch;
	ctx->sounds[35] = &sound_empty;
	ctx->sounds[36] = &sound_intro;

	ctx->sounds[37] = &sound_debris;
	ctx->sounds[38] = &sound_bounce;
	ctx->sounds[39] = &sound_impact;

	// Initialize sound names
	strcpy(ctx->sound_names[0], "sound_footstep1");
	strcpy(ctx->sound_names[1], "sound_footstep2");
	strcpy(ctx->sound_names[2], "sound_footstep3");
	strcpy(ctx->sound_names[3], "sound_footstep4");

	strcpy(ctx->sound_names[4], "sound_wade1");
	strcpy(ctx->sound_names[5], "sound_wade2");
	strcpy(ctx->sound_names[6], "sound_wade3");
	strcpy(ctx->sound_names[7], "sound_wade4");

	strcpy(ctx->sound_names[8], "sound_jump");
	strcpy(ctx->sound_names[9], "sound_jump_water");

	strcpy(ctx->sound_names[10], "sound_land");
	strcpy(ctx->sound_names[11], "sound_land_water");

	strcpy(ctx->sound_names[12], "sound_hurt_fall");

	strcpy(ctx->sound_names[13], "sound_explode");
	strcpy(ctx->sound_names[14], "sound_explode_water");
	strcpy(ctx->sound_names[15], "sound_grenade_bounce");
	strcpy(ctx->sound_names[16], "sound_grenade_pin");

	strcpy(ctx->sound_names[17], "sound_pickup");
	strcpy(ctx->sound_names[18], "sound_horn");

	strcpy(ctx->sound_names[19], "sound_rifle_shoot");
	strcpy(ctx->sound_names[20], "sound_rifle_reload");
	strcpy(ctx->sound_names[21], "sound_smg_shoot");
	strcpy(ctx->sound_names[22], "sound_smg_reload");
	strcpy(ctx->sound_names[23], "sound_shotgun_shoot");
	strcpy(ctx->sound_names[24], "sound_shotgun_reload");
	strcpy(ctx->sound_names[25], "sound_shotgun_cock");

	strcpy(ctx->sound_names[26], "sound_hitground");
	strcpy(ctx->sound_names[27], "sound_hitplayer");
	strcpy(ctx->sound_names[28], "sound_build");

	strcpy(ctx->sound_names[29], "sound_spade_woosh");
	strcpy(ctx->sound_names[30], "sound_spade_whack");

	strcpy(ctx->sound_names[31], "sound_death");
	strcpy(ctx->sound_names[32], "sound_beep1");
	strcpy(ctx->sound_names[33], "sound_beep2");
	strcpy(ctx->sound_names[34], "sound_switch");
	strcpy(ctx->sound_names[35], "sound_empty");
	strcpy(ctx->sound_names[36], "sound_intro");

	strcpy(ctx->sound_names[37], "sound_debris");
	strcpy(ctx->sound_names[38], "sound_bounce");
	strcpy(ctx->sound_names[39], "sound_impact");

	log_info("Successfully initialized soundtest context.");
#endif
#endif
}

void soundtest_clean(struct soundtest_ctx * ctx)
{
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	log_info("Freeing soundtest context...");
	for (int i = 0; i < ctx->n_sounds; i++)
		free(ctx->sound_names[i]);
	free(ctx->sound_names);
	free(ctx->sounds);
	log_info("Soundtest context freed successfully.");
#endif
#endif
}

void soundtest_play_current(struct soundtest_ctx * ctx)
{
#ifdef USE_SOUNDTEST
#ifdef USE_SOUND
	sound_create(NULL, SOUND_LOCAL, ctx->sounds[ctx->current_sound], 0.0F, 0.0F, 0.0F);
	log_info("Playing sound '%s'", ctx->sound_names[ctx->current_sound]);
#endif
#endif
}

struct soundtest_ctx st_ctx = {
	0,
	0,
	(char **)NULL,
	(struct Sound_wav **)NULL,
	soundtest_init,
	soundtest_clean,
	soundtest_next_sound,
	soundtest_prev_sound,
	soundtest_play_current
};
