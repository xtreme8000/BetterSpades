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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>

#include "common.h"
#include "log.h"
#include "file.h"

struct file_handle {
	void* internal;
	int type;
};

enum { FILE_STD, FILE_SDL };

void file_url(char* url) {
	char cmd[strlen(url) + 16];
#ifdef OS_WINDOWS
	sprintf(cmd, "start %s", url);
	system(cmd);
#endif
#if defined(OS_LINUX) || defined(OS_APPLE)
	sprintf(cmd, "open %s", url);
	system(cmd);
#endif
}

int file_dir_exists(const char* path) {
#ifndef USE_ANDROID_FILE
	DIR* d = opendir(path);
#else
	char str[256];
	sprintf(str, "/sdcard/BetterSpades/%s", path);
	DIR* d = opendir(str);
#endif
	if(d) {
		closedir(d);
		return 1;
	} else {
		return 0;
	}
}

int file_dir_create(const char* path) {
#ifndef USE_ANDROID_FILE
	mkdir(path);
#else
	char str[256];
	sprintf(str, "/sdcard/BetterSpades/%s", path);
	mkdir(str);
#endif
	return 1;
}

int file_exists(const char* name) {
#ifdef USE_ANDROID_FILE
	void* f = file_open(name, "rb");
	if(f == NULL)
		return 0;
	file_close(f);
	return 1;
#else
	return !access(name, F_OK);
#endif
}

int file_size(const char* name) {
#ifdef USE_ANDROID_FILE
	struct file_handle* f = (struct file_handle*)file_open(name, "rb");
	if(!f)
		return 0;
	if(f->type == FILE_SDL) {
		int size = SDL_RWsize((struct SDL_RWops*)f->internal);
		file_close(f);
		return size;
	}
	if(f->type == FILE_STD) {
		fseek(f->internal, 0, SEEK_END);
		int size = ftell(f->internal);
		file_close(f);
		return size;
	}
	return 0;
#else
	FILE* f = fopen(name, "rb");
	if(!f)
		return 0;
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fclose(f);
	return size;
#endif
}

unsigned char* file_load(const char* name) {
#ifdef USE_ANDROID_FILE
	int size = file_size(name);
	struct file_handle* f = (struct file_handle*)file_open(name, "rb");
	if(!f)
		return NULL;
	unsigned char* data = malloc(size + 1);
	CHECK_ALLOCATION_ERROR(data)
	data[size] = 0;
	if(f->type == FILE_SDL) {
		int offset = 0;
		while(1) {
			int read = SDL_RWread((struct SDL_RWops*)f->internal, data + offset, 1, size - offset);
			if(!read)
				break;
			offset += read;
		}
		SDL_RWclose((struct SDL_RWops*)f->internal);
		if(!offset) {
			free(data);
			return NULL;
		}
	}
	if(f->type == FILE_STD) {
		fread(data, size, 1, f->internal);
		fclose(f->internal);
	}
	return data;
#else
	FILE* f;
	f = fopen(name, "rb");
	if(!f) {
		log_fatal("ERROR: failed to open '%s', exiting", name);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	unsigned char* data = malloc(size + 1);
	CHECK_ALLOCATION_ERROR(data)
	data[size] = 0;
	fseek(f, 0, SEEK_SET);
	fread(data, size, 1, f);
	fclose(f);
	return data;
#endif
}

void* file_open(const char* name, const char* mode) {
#ifdef USE_ANDROID_FILE
	struct file_handle* handle = malloc(sizeof(struct file_handle));
	handle->internal = (strchr(mode, 'r') != NULL) ? SDL_RWFromFile(name, mode) : NULL;
	handle->type = FILE_SDL;
	if(!handle->internal) {
		char str[256];
		sprintf(str, "/sdcard/BetterSpades/%s", name);
		handle->internal = fopen(str, mode);
		handle->type = FILE_STD;
		// log_warn("open %s %i",str,handle->internal);
	}
	if(!handle->internal) {
		free(handle);
		return NULL;
	}
	return handle;
#else
	return fopen(name, mode);
#endif
}

void file_printf(void* file, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
#ifdef USE_ANDROID_FILE
	struct file_handle* f = (struct file_handle*)file;
	if(f->type == FILE_SDL) {
		char str[256];
		vsprintf(str, fmt, args);
		int written = 0;
		int total = strlen(str);
		while(written < total)
			written += SDL_RWwrite((struct SDL_RWops*)f->internal, str + written, 1, total - written);
	}
	if(f->type == FILE_STD) {
		log_warn("%i %i", f->internal, f);
		vfprintf((FILE*)f->internal, fmt, args);
	}
#else
	vfprintf((FILE*)file, fmt, args);
#endif
	va_end(args);
}

void file_close(void* file) {
#ifdef USE_ANDROID_FILE
	struct file_handle* f = (struct file_handle*)file;
	if(f->type == FILE_SDL) {
		SDL_RWclose((struct SDL_RWops*)f->internal);
	}
	if(f->type == FILE_STD) {
		fclose((FILE*)f->internal);
	}
	free(f);
#else
	fclose((FILE*)file);
#endif
}

float buffer_readf(unsigned char* buffer, int index) {
	return ((float*)(buffer + index))[0];
}

unsigned int buffer_read32(unsigned char* buffer, int index) {
	return (buffer[index + 3] << 24) | (buffer[index + 2] << 16) | (buffer[index + 1] << 8) | buffer[index];
}

unsigned short buffer_read16(unsigned char* buffer, int index) {
	return (buffer[index + 1] << 8) | buffer[index];
}

unsigned char buffer_read8(unsigned char* buffer, int index) {
	return buffer[index];
}
