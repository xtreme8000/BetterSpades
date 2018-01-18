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

char file_exists(const char* name);
unsigned char* file_load(const char* name);
float buffer_readf(unsigned char* buffer, int index);
unsigned int buffer_read32(unsigned char* buffer, int index);
unsigned short buffer_read16(unsigned char* buffer, int index);
unsigned char buffer_read8(unsigned char* buffer, int index);

#ifdef OS_WINDOWS
#define file_url(url) system("start "url)
#endif

#if defined(OS_LINUX) ||  defined(OS_APPLE)
#define file_url(url) system("open "url)
#endif
