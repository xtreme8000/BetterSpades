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

#include <enet/enet.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include "window.h"
#include "ping.h"
#include "common.h"
#include "parson.h"
#include "list.h"
#include "hud.h"
#include "channel.h"
#include "hashtable.h"
#include "utils.h"

struct channel ping_queue;
ENetSocket sock, lan;
pthread_t ping_thread;
void (*ping_result)(void*, float time_delta, char* aos);

void ping_init() {
	channel_create(&ping_queue, sizeof(struct ping_entry), 64);

	sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);

	lan = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(lan, ENET_SOCKOPT_NONBLOCK, 1);
	enet_socket_set_option(lan, ENET_SOCKOPT_BROADCAST, 1);

	pthread_create(&ping_thread, NULL, ping_update, NULL);
}

void ping_deinit() {
	enet_socket_destroy(sock);
	enet_socket_destroy(lan);
}

static void ping_lan() {
	ENetAddress addr = {.host = 0xFFFFFFFF}; // 255.255.255.255

	ENetBuffer buffer = {
		.data = "HELLOLAN",
		.dataLength = 8,
	};

	for(addr.port = 32882; addr.port < 32892; addr.port++)
		enet_socket_send(lan, &addr, &buffer, 1);
}

static bool pings_retry(void* key, void* value, void* user) {
	struct ping_entry* entry = (struct ping_entry*)value;

	if(window_time() - entry->time_start > 2.0F) { // timeout
		// try up to 3 times after first failed attempt
		if(entry->trycount >= 3) {
			return true;
		} else {
			enet_socket_send(sock, &entry->addr, &(ENetBuffer) {.data = "HELLO", .dataLength = 5}, 1);
			entry->time_start = window_time();
			entry->trycount++;
			log_warn("Ping timeout on %s, retrying", entry->aos);
		}
	}

	return false;
}

#define IP_KEY(addr) (((uint64_t)addr.host << 16) | (addr.port));

void* ping_update(void* data) {
	pthread_detach(pthread_self());

	ping_lan();
	float ping_start = window_time();

	HashTable pings;
	ht_setup(&pings, sizeof(uint64_t), sizeof(struct ping_entry), 64);

	while(1) {
		if (!ping_result)
			continue;

		size_t drain = channel_size(&ping_queue);
		for(size_t k = 0; (k < drain) || (!pings.size && window_time() - ping_start >= 8.0F); k++) {
			struct ping_entry entry;
			channel_await(&ping_queue, &entry);

			uint64_t ID = IP_KEY(entry.addr);
			ht_insert(&pings, &ID, &entry);
		}

		char tmp[512];
		ENetAddress from;

		ENetBuffer buf = {
			.data = tmp,
			.dataLength = sizeof(tmp),
		};

		while(1) {
			int recvLength = enet_socket_receive(sock, &from, &buf, 1);
			uint64_t ID = IP_KEY(from);

			if(recvLength != 0) {
				struct ping_entry* entry = ht_lookup(&pings, &ID);

				if(entry) {
					if(recvLength > 0) { // received something!
						if(!strncmp(buf.data, "HI", recvLength)) {
							ping_result(NULL, window_time() - entry->time_start, entry->aos);
							ht_erase(&pings, &ID);
						} else {
							entry->trycount++;
						}
					} else { // connection was closed
						ht_erase(&pings, &ID);
					}
				}
			} else { // would block
				break;
			}
		}

		ht_iterate_remove(&pings, NULL, pings_retry);

		int length = enet_socket_receive(lan, &from, &buf, 1);
		if(length) {
			JSON_Value* js = json_parse_string(buf.data);
			if(js) {
				JSON_Object* root = json_value_get_object(js);

				struct serverlist_entry e;

				strcpy(e.country, "LAN");
				e.ping = ceil((window_time() - ping_start) * 1000.0F);
				snprintf(e.identifier, sizeof(e.identifier) - 1, "aos://%u:%u", from.host, from.port);

				strncpy(e.name, json_object_get_string(root, "name"), sizeof(e.name) - 1);
				e.name[sizeof(e.name) - 1] = 0;
				strncpy(e.gamemode, json_object_get_string(root, "game_mode"), sizeof(e.gamemode) - 1);
				e.gamemode[sizeof(e.gamemode) - 1] = 0;
				strncpy(e.map, json_object_get_string(root, "map"), sizeof(e.map) - 1);
				e.map[sizeof(e.map) - 1] = 0;
				e.current = json_object_get_number(root, "players_current");
				e.max = json_object_get_number(root, "players_max");
				ping_result(&e, window_time() - ping_start, NULL);

				json_value_free(js);
			}
		}

		usleep(1000000);
	}
}

void ping_check(char* addr, int port, char* aos) {
	struct ping_entry entry = {
		.trycount = 0,
		.addr.port = port,
		.time_start = window_time(),
	};

	strncpy(entry.aos, aos, sizeof(entry.aos) - 1);
	entry.aos[sizeof(entry.aos) - 1] = 0;

	enet_address_set_host(&entry.addr, addr);

	channel_put(&ping_queue, &entry);

	enet_socket_send(sock, &entry.addr, &(ENetBuffer) {.data = "HELLO", .dataLength = 5}, 1);
}

void ping_start(void (*result)(void*, float, char*)) {
	ping_stop();

	ping_result = result;
}

void ping_stop() {
	channel_clear(&ping_queue);
}

