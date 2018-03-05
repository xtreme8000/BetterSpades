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

//for future opengl es abstraction layer

void glx_init(void);

void glx_enable_sphericalfog(void);
void glx_disable_sphericalfog(void);

int glx_displaylist_create(void);
void glx_displaylist_update(int id, int size, unsigned char* color, short* vertex);
void glx_displaylist_draw(int id, int size);
