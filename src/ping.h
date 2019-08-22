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

struct ping_entry {
	ENetAddress addr;
	float time_start;
	char user_data[32];
	int trycount;
};

void ping_init();
void ping_deinit();
void ping_check(char* addr, int port, void* user_data);
void* ping_update(void* data);
void ping_start(void (*finished) (),  void (*result) (void*, float, void*));
void ping_lan();
void ping_stop();
