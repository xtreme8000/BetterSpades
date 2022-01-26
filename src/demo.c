/*
Demo recording compatible with aos_replay (https://github.com/BR-/aos_replay)
*/

#include "stdio.h"
#include "demo.h"
#include "time.h"
#include "string.h"
#include "enet/enet.h"
#include "window.h"

struct Demo CurrentDemo;
static const struct Demo ResetStruct;

FILE* create_demo_file(char server_name[64]) {
	time_t current_time = time(NULL);
	char* str_time = ctime(&current_time);

	char file_name[100] = ""; // demos_dir+Server_name(-)+str_time+extension

	char* p_char = strchr(server_name, '/');
	while (p_char!=NULL) {
		server_name[p_char-server_name] = ' ';
		p_char = strchr(server_name, '/');
	}

	strncat(file_name, "demos/", 6);
	strncat(file_name, server_name, 64);
	strcat(file_name, " ");
	strncat(file_name, str_time, 24);
	strcat(file_name, ".dem");

	FILE* file;
	file = fopen(file_name, "wb");
	// aos_replay version + 0.75 version
	unsigned char value = 1;
	fwrite(&value, sizeof(value), 1, file);

	value = 3;
	fwrite(&value, sizeof(value), 1, file);	

	return file;
}

void register_demo_packet(ENetPacket *packet) {
	if (!CurrentDemo.fp)
		return;

	float c_time = time(NULL)-CurrentDemo.start_time;
	unsigned short len = packet->dataLength;

	fwrite(&c_time, sizeof(c_time), 1, CurrentDemo.fp);
	fwrite(&len, sizeof(len), 1, CurrentDemo.fp);

	fwrite(packet->data, packet->dataLength, 1, CurrentDemo.fp);
}


void demo_start_record(char server_name[64]) {
	CurrentDemo.fp = create_demo_file(server_name);
	CurrentDemo.start_time = time(NULL);
}

void demo_stop_record() {
	if(CurrentDemo.fp)
		fclose(CurrentDemo.fp);

	CurrentDemo = ResetStruct;
}