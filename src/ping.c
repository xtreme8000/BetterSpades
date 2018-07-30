#include "common.h"

struct list list_pings;
ENetSocket sock;
pthread_t ping_thread;
pthread_mutex_t list_ping_lock;
void (*ping_finished) ();

void ping_init() {
	list_create(&list_pings,sizeof(struct ping_entry));
	sock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
	enet_socket_set_option(sock,ENET_SOCKOPT_NONBLOCK,1);
	enet_socket_set_option(sock,ENET_SOCKOPT_NODELAY,1);
	pthread_mutex_init(&list_ping_lock,NULL);
}

void ping_deinit() {
	enet_socket_destroy(sock);
}

void* ping_update(void* data) {
	while(1) {
		ENetBuffer buf;
		buf.dataLength = 32;

		pthread_mutex_lock(&list_ping_lock);
		for(int k=list_size(&list_pings)-1;k>=0;k--) {
			struct ping_entry* entry = list_get(&list_pings,k);

			if(window_time()-entry->time_start>3.0F) { //timeout
				if(entry->trycount<3) {
					log_warn("Ping timeout, retrying (%i)",entry->trycount);
					ENetBuffer buffer;
					buffer.data = "HELLO";
					buffer.dataLength = 5;
					entry->time_start = window_time();
					enet_socket_send(sock,&entry->addr,&buffer,1);
					entry->trycount++;
				} else {
					list_remove(&list_pings,k);
					continue;
				}
			}

			char tmp[32] = {0};
			buf.data = tmp;

			switch(enet_socket_receive(sock,&entry->addr,&buf,1)) {
				case 0: //would block
					break;
				default: //received something!
					if(strcmp(buf.data,"HI")==0 && entry->callback)
						entry->callback(entry,window_time()-entry->time_start,entry->user_data);
				case -1: //connection was closed
					list_remove(&list_pings,k);
			}
		}

		int size = list_size(&list_pings);
		pthread_mutex_unlock(&list_ping_lock);
		if(!size) {
			ping_finished();
			return NULL;
		}
	}
}

void ping_check(char* addr, int port, void* user_data, void (*callback) (struct ping_entry*, float time_delta, void* user_data)) {
	struct ping_entry* entry = list_add(&list_pings,NULL);

	entry->callback = callback;
	entry->user_data = user_data;
	entry->trycount = 1;

	enet_address_set_host(&entry->addr,addr);
	entry->addr.port = port;

	ENetBuffer buffer;
	buffer.data = "HELLO";
	buffer.dataLength = 5;

	entry->time_start = window_time();
	enet_socket_send(sock,&entry->addr,&buffer,1);
}

void ping_start(void (*callback) ()) {
	pthread_create(&ping_thread,NULL,ping_update,NULL);
	ping_finished = callback;
}
