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

#ifndef FILE_H
#define FILE_H

void* file_open(const char* name, const char* mode);
void file_printf(void* file, const char* fmt, ...);
void file_close(void* file);
int file_size(const char* name);
int file_dir_exists(const char* path);
int file_dir_create(const char* path);
int file_exists(const char* name);
unsigned char* file_load(const char* name);
float buffer_readf(unsigned char* buffer, int index);
unsigned int buffer_read32(unsigned char* buffer, int index);
unsigned short buffer_read16(unsigned char* buffer, int index);
unsigned char buffer_read8(unsigned char* buffer, int index);
void file_url(char* url);

#endif
