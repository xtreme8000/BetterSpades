#include "common.h"

char file_exists(const char* name) {
	return !access(name,F_OK);
}

unsigned char* file_load(const char* name) {
	FILE* f;
	f = fopen(name,"rb");
	if (!f) {
		printf("ERROR: failed to open '%s', exiting\n", name);
		exit(1);
	}
	fseek(f,0,SEEK_END);
	int size = ftell(f);
	unsigned char* data = malloc(size+1);
	data[size] = 0;
	fseek(f,0,SEEK_SET);
	fread(data,size,1,f);
	fclose(f);
	return data;
}

float buffer_readf(unsigned char* buffer, int index) {
	return ((float*)(buffer+index))[0];
}

unsigned int buffer_read32(unsigned char* buffer, int index) {
	return (buffer[index+3]<<24) | (buffer[index+2]<<16) | (buffer[index+1]<<8) | buffer[index];
}

unsigned short buffer_read16(unsigned char* buffer, int index) {
	return (buffer[index+1]<<8) | buffer[index];
}

unsigned char buffer_read8(unsigned char* buffer, int index) {
	return buffer[index];
}
