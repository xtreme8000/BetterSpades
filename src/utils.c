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

#include "utils.h"

static int base64_map(char c) {
	if(c >= '0' && c <= '9')
		return c + 4;
	if(c >= 'A' && c <= 'Z')
		return c - 'A';
	if(c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	if(c == '+')
		return 62;
	if(c == '/')
		return 63;
	return 0;
}

// works in place
int base64_decode(char* data, int len) {
	int buffer = 0;
	int buffer_len = 0;
	int data_index = 0;
	int out_index = 0;
	for(int k = 0; k < len; k++) {
		if(buffer_len + 6 < sizeof(int) * 8 && data[data_index] != '=') {
			buffer <<= 6;
			buffer |= base64_map(data[data_index++]);
			buffer_len += 6;
		}
		for(int b = 0; b < buffer_len / 8; b++) {
			data[out_index++] = (buffer >> (buffer_len - 8));
			buffer_len -= 8;
		}
	}
	return out_index;
}
