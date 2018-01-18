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
