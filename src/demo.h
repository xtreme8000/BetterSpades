#include "stdio.h"
#include "enet/enet.h"
#include "time.h"

struct Demo {
	FILE* fp;
	time_t start_time;
};

FILE* create_demo_file(char server_name[64]);
void register_demo_packet(ENetPacket *packet);
void demo_start_record(char server_name[64]);
void demo_stop_record();