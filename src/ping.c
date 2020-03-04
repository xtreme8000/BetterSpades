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
#include "parson.h"

struct list list_pings;
ENetSocket sock, lan;
pthread_t ping_thread;
pthread_mutex_t ping_lock;
void (*ping_finished)();
void (*ping_result)(void*, float time_delta, void* user_data);

void ping_init() {
	list_create(&list_pings, sizeof(struct ping_entry));

	sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(sock, ENET_SOCKOPT_NONBLOCK, 1);

	lan = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(lan, ENET_SOCKOPT_NONBLOCK, 1);
	enet_socket_set_option(lan, ENET_SOCKOPT_BROADCAST, 1);

	pthread_mutex_init(&ping_lock, NULL);
}

void ping_deinit() {
	pthread_mutex_lock(&ping_lock); // stop the ping thread from running
	enet_socket_destroy(sock);
	enet_socket_destroy(lan);
}

void* ping_update(void* data) {
	ping_lan();
	float ping_start = window_time();

	while(1) {
		char tmp[512];

		ENetBuffer buf;
		buf.data = tmp;

		ENetAddress from;

		pthread_mutex_lock(&ping_lock);

		while(1) {
			buf.dataLength = 512;
			memset(tmp, 0, buf.dataLength);
			int response = enet_socket_receive(sock, &from, &buf, 1);
			if(response != 0) {
				struct ping_entry* entry;
				int k;
				for(k = list_size(&list_pings) - 1; k >= 0; k--) {
					entry = list_get(&list_pings, k);
					if(entry->addr.host == from.host && entry->addr.port == from.port)
						break;
				}

				switch(response) {
					default: // received something!
						if(strcmp(buf.data, "HI") == 0) {
							ping_result(NULL, window_time() - entry->time_start, entry->user_data);
						}
					case -1: // connection was closed
						list_remove(&list_pings, k);
				}
			} else { // would block
				break;
			}
		}

		int retry = 0;
		for(int k = list_size(&list_pings) - 1; k >= 0; k--) {
			struct ping_entry* entry = list_get(&list_pings, k);

			if(window_time() - entry->time_start > 2.0F) { // timeout
				if(entry->trycount < 3) {				   // try up to 3 times after first failed attempt
					retry = 1;
					ENetBuffer buffer;
					buffer.data = "HELLO";
					buffer.dataLength = 5;
					entry->time_start = window_time();
					enet_socket_send(sock, &entry->addr, &buffer, 1);
					entry->trycount++;
				} else {
					list_remove(&list_pings, k);
					continue;
				}
			}
		}

		if(retry)
			log_warn("Ping timeout on some server(s), retrying");

		buf.dataLength = 512;
		memset(tmp, 0, buf.dataLength);
		int length = enet_socket_receive(lan, &from, &buf, 1);
		if(length) {
			struct serverlist_entry e;

			strcpy(e.country, "LAN");
			e.ping = ceil((window_time() - ping_start) * 1000.0F);
			snprintf(e.identifier, sizeof(e.identifier) - 1, "aos://%i:%i", from.host, from.port);

			JSON_Value* js = json_parse_string(tmp);
			JSON_Object* root = json_value_get_object(js);
			strncpy(e.name, json_object_get_string(root, "name"), sizeof(e.name) - 1);
			strncpy(e.gamemode, json_object_get_string(root, "game_mode"), sizeof(e.gamemode) - 1);
			strncpy(e.map, json_object_get_string(root, "map"), sizeof(e.map) - 1);
			e.current = json_object_get_number(root, "players_current");
			e.max = json_object_get_number(root, "players_max");
			json_value_free(js);
			ping_result(&e, window_time() - ping_start, NULL);
		}

		if(!list_size(&list_pings)) {
			ping_finished();
			log_info("Ping thread stopped");
			pthread_mutex_unlock(&ping_lock);
			return NULL;
		}

		pthread_mutex_unlock(&ping_lock);

		usleep(1);
	}
}

void ping_lan() {
	ENetAddress addr;
	addr.host = 0xFFFFFFFF; // 255.255.255.255

	ENetBuffer buffer;
	buffer.data = "HELLOLAN";
	buffer.dataLength = 8;
	for(addr.port = 32880; addr.port < 32890; addr.port++)
		enet_socket_send(lan, &addr, &buffer, 1);
}

void ping_check(char* addr, int port, void* user_data) {
	pthread_mutex_lock(&ping_lock);
	struct ping_entry* entry = list_add(&list_pings, NULL);

	strcpy(entry->user_data, user_data);
	entry->trycount = 0;

	enet_address_set_host(&entry->addr, addr);
	entry->addr.port = port;

	ENetBuffer buffer;
	buffer.data = "HELLO";
	buffer.dataLength = 5;

	entry->time_start = window_time();
	enet_socket_send(sock, &entry->addr, &buffer, 1);
	pthread_mutex_unlock(&ping_lock);
}

void ping_start(void (*finished)(), void (*result)(void*, float, void*)) {
	pthread_create(&ping_thread, NULL, ping_update, NULL);
	ping_finished = finished;
	ping_result = result;
}

void ping_stop() {
	pthread_mutex_lock(&ping_lock);
	if(list_created(&list_pings))
		list_clear(&list_pings);
	pthread_mutex_unlock(&ping_lock);
}
