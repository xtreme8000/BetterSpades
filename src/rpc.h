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

#ifndef RPC_H
#define RPC_H

enum RPC_VALUE {
	RPC_VALUE_SERVERNAME,
	RPC_VALUE_PLAYERS,
	RPC_VALUE_SLOTS,
	RPC_VALUE_SERVERURL,
};

void rpc_init(void);
void rpc_deinit(void);
void rpc_setv(enum RPC_VALUE v, char* x);
void rpc_seti(enum RPC_VALUE v, int x);
void rpc_update(void);

#endif
