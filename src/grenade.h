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

#define GRENADES_MAX PLAYERS_MAX*3
//each can throw all their grenades all at once

struct Grenade {
    unsigned char active;
    unsigned char owner;
    float fuse_length;
    float created;
    struct Position pos;
    struct Velocity velocity;
};

struct Grenade* grenade_add(void);
void grenade_update(float dt);
int grenade_inwater(struct Grenade* g);
