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
#include "utils.h"

#define IP_KEY(addr) (((uint64_t)addr.host << 16) | (addr.port))

struct ping_task {
	HashTable* servers;
	void (*ping_result)(void*, float time_delta, char* aos);
	bool quit;
};

struct channel ping_queue;
struct channel ping_lan_queue;
ENetSocket sock, lan;
pthread_t ping_thread;
pthread_t ping_lan_thread;

static bool pings_retry(void* key, void* value, void* user) {
	struct ping_entry* entry = (struct ping_entry*)value;
	int* retry = (int*)user;

	enet_socket_send(sock, &entry->addr, &(ENetBuffer) {.data = "HELLO", .dataLength = 5}, 1);
	entry->time_start = window_time();

	if(*retry > 0)
		log_warn("Ping timeout on %s, retrying (%i)", entry->aos, *retry);

	return false;
}

void* ping_update(void* data) {
	pthread_detach(pthread_self());

	ENetAddress from;
	uint8_t tmp[512];

	ENetBuffer buf = {
		.data = tmp,
		.dataLength = sizeof(tmp),
	};

	while(1) {
		struct ping_task task;
		channel_await(&ping_queue, &task);

		if(task.quit)
			break;

		ht_iterate_remove(task.servers, (int[]) {0}, pings_retry);

		for(int try = 0; try < 3; try++) {
			float ping_start = window_time();

			while(1) {
				int timeout = (2.5F - (window_time() - ping_start)) * 1000;

				if(timeout <= 0)
					break;

				enet_uint32 event = ENET_SOCKET_WAIT_RECEIVE;
				enet_socket_wait(sock, &event, timeout);
				size_t recv_length = enet_socket_receive(sock, &from, &buf, 1);
				uint64_t ID = IP_KEY(from);

				if(recv_length != 0) {
					struct ping_entry* entry = ht_lookup(task.servers, &ID);

					if(entry) {
						if(recv_length > 0) { // received something!
							if(!strncmp(buf.data, "HI", recv_length)) {
								task.ping_result(NULL, window_time() - entry->time_start, entry->aos);
								ht_erase(task.servers, &ID);
							}
						} else { // connection was closed
							ht_erase(task.servers, &ID);
						}
					}
				}
			}

			ht_iterate_remove(task.servers, (int[]) {try}, pings_retry);

			if(ht_is_empty(task.servers))
				break;
		}

		if(!ht_is_empty(task.servers))
			log_warn("Some servers did not reply to ping requests");

		ht_destroy(task.servers);
		free(task.servers);
	}

	enet_socket_destroy(sock);

	return NULL;
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

void* ping_lan_update(void* data) {
	pthread_detach(pthread_self());

	ENetAddress from;
	uint8_t tmp[512];

	ENetBuffer buf = {
		.data = tmp,
		.dataLength = sizeof(tmp),
	};

	while(1) {
		struct ping_task task;
		channel_await(&ping_lan_queue, &task);

		if(task.quit)
			break;

		float ping_start = window_time();

		ping_lan();

		bool found = false;

		while(1) {
			int timeout = (8.0F - (window_time() - ping_start)) * 1000;

			if(timeout <= 0)
				break;

			enet_uint32 event = ENET_SOCKET_WAIT_RECEIVE;
			enet_socket_wait(sock, &event, timeout);

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
					task.ping_result(&e, window_time() - ping_start, NULL);
					found = true;

					json_value_free(js);
				}
			}
		}

		if(!found)
			log_info("No local servers found");
	}

	enet_socket_destroy(lan);

	return NULL;
}

void ping_check(HashTable* pings, char* addr, int port, char* aos) {
	struct ping_entry entry;

	strncpy(entry.aos, aos, sizeof(entry.aos) - 1);
	entry.aos[sizeof(entry.aos) - 1] = 0;

	enet_address_set_host(&entry.addr, addr);
	entry.addr.port = port;

	ht_insert(pings, (uint64_t[]) {IP_KEY(entry.addr)}, &entry);
}

void ping_start(HashTable* pings, void (*result)(void*, float, char*)) {
	ping_stop();

	channel_put(&ping_queue,
				&(struct ping_task) {
					.servers = pings,
					.ping_result = result,
					.quit = false,
				});

	channel_put(&ping_lan_queue,
				&(struct ping_task) {
					.ping_result = result,
					.quit = false,
				});
}

void ping_stop() {
	channel_clear(&ping_queue);
	channel_clear(&ping_lan_queue);
}

void ping_init() {
	channel_create(&ping_queue, sizeof(struct ping_task), 4);
	channel_create(&ping_lan_queue, sizeof(struct ping_task), 4);

	sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);

	lan = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(lan, ENET_SOCKOPT_NONBLOCK, 1);
	enet_socket_set_option(lan, ENET_SOCKOPT_BROADCAST, 1);

	pthread_create(&ping_thread, NULL, ping_update, NULL);
	pthread_create(&ping_lan_thread, NULL, ping_lan_update, NULL);
}

void ping_deinit() {
	ping_stop();
	channel_put(&ping_queue, &(struct ping_task) {.quit = true});
	channel_put(&ping_lan_queue, &(struct ping_task) {.quit = true});
}
