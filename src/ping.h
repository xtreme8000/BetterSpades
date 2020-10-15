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

#ifndef PING_H
#define PING_H

#include <enet/enet.h>

struct ping_entry {
	ENetAddress addr;
	char aos[64];
	float time_start;
	int trycount;
};

void ping_init();
void ping_deinit();
void ping_check(char* addr, int port, char* aos);
void* ping_update(void* data);
void ping_start(void (*result)(void*, float, char*));
void ping_stop();

#endif
