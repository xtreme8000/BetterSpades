/*
	Copyright (c) 2017-2018 ByteBit

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

#define TRACER_MAX PLAYERS_MAX*5

struct Tracer {
    Ray r;
    unsigned char type;
    float created;
    unsigned char used;
};

extern struct Tracer* tracers;

void tracer_pvelocity(float* o, struct Player* p);
void tracer_add(unsigned char type, float x, float y, float z, float dx, float dy, float dz);
void tracer_update(float dt);
void tracer_render(void);
void tracer_init(void);
