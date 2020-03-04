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

#include "common.h"

struct rpc {
	int needs_update;
	int players;
	int slots;
	char server_url[64];
	char server_name[64];
} rpc_state;

#ifdef USE_RPC
static void rpc_joingame(const char* joinSecret) {
	log_info("discord join %s!", joinSecret);
}

static void rpc_joinrequest(const DiscordUser* request) {
	Discord_Respond(request->userId, DISCORD_REPLY_YES);
}
#endif

void rpc_init() {
#ifdef USE_RPC
	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));
	handlers.joinGame = rpc_joingame;
	handlers.joinRequest = rpc_joinrequest;
	Discord_Initialize("DISCORD TOKEN HERE", &handlers, 1, NULL);
#endif
	rpc_state.needs_update = 1;
	rpc_state.players = 0;
	rpc_state.slots = 0;
	*rpc_state.server_url = 0;
	*rpc_state.server_name = 0;
}

void rpc_deinit() {
#ifdef USE_RPC
	Discord_Shutdown();
#endif
}

void rpc_setv(enum RPC_VALUE v, char* x) {
	switch(v) {
		case RPC_VALUE_SERVERNAME:
			if(strcmp(rpc_state.server_name, x) != 0) {
				strncpy(rpc_state.server_name, x, sizeof(rpc_state.server_name) - 1);
				rpc_state.needs_update = 1;
			}
			break;
		case RPC_VALUE_SERVERURL:
			if(strcmp(rpc_state.server_url, x) != 0) {
				strncpy(rpc_state.server_url, x, sizeof(rpc_state.server_url) - 1);
				rpc_state.needs_update = 1;
			}
			break;
	}
}

void rpc_seti(enum RPC_VALUE v, int x) {
	switch(v) {
		case RPC_VALUE_PLAYERS:
			if(rpc_state.players != x) {
				rpc_state.players = x;
				rpc_state.needs_update = 1;
			}
			break;
		case RPC_VALUE_SLOTS:
			if(rpc_state.slots != x) {
				rpc_state.slots = x;
				rpc_state.needs_update = 1;
			}
			break;
	}
}

void rpc_update() {
#ifdef USE_RPC

	int online = 0;
	for(int k = 0; k < PLAYERS_MAX; k++) {
		if(players[k].connected)
			online++;
	}
	rpc_seti(RPC_VALUE_PLAYERS, online);

	if(rpc_state.needs_update) {
		DiscordRichPresence discordPresence;
		memset(&discordPresence, 0, sizeof(discordPresence));
		discordPresence.largeImageKey = "pic03";
		discordPresence.smallImageKey = "logo";
		discordPresence.smallImageText = BETTERSPADES_VERSION;
		discordPresence.instance = 1;
		if(rpc_state.slots > 0) {
			discordPresence.state = "Playing";
			discordPresence.partyId = "42";
			discordPresence.partySize = max(rpc_state.players, 1);
			discordPresence.partyMax = rpc_state.slots;
			discordPresence.details = rpc_state.server_name;
			discordPresence.joinSecret = rpc_state.server_url;
		} else {
			discordPresence.state = "Waiting";
		}
		Discord_UpdatePresence(&discordPresence);
		rpc_state.needs_update = 0;
	}

	Discord_UpdateConnection();
	Discord_RunCallbacks();
#endif
}
