#ifndef SOUNDTEST_H
#define SOUNDTEST_H

#include "sound.h"

struct soundtest_ctx {
	int current_sound;
	int n_sounds;
	char ** sound_names;
	struct Sound_wav ** sounds;
	void (*init)(struct soundtest_ctx *);
	void (*clean)(struct soundtest_ctx *);
	void (*next_sound)(struct soundtest_ctx *);
	void (*prev_sound)(struct soundtest_ctx *);
	void (*play_current)(struct soundtest_ctx *);
};

extern struct soundtest_ctx st_ctx;

#endif
