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

struct texture font_fixedsys;
struct texture font_smallfnt;

short* font_vertex_buffer;
short* font_coords_buffer;
int font_type = FONT_FIXEDSYS;

unsigned char font_init() {
    texture_create(&font_fixedsys,"fonts/FixedSys_Bold_36.png");
    texture_create(&font_smallfnt,"fonts/smallfnt68.png");

    font_vertex_buffer = malloc(512*8*sizeof(short));
    CHECK_ALLOCATION_ERROR(font_vertex_buffer)
    font_coords_buffer = malloc(512*8*sizeof(short));
    CHECK_ALLOCATION_ERROR(font_coords_buffer)

    return 1;
}

void font_select(char type) {
    font_type = type;
}

float font_length(float h, char* text) {
    return strlen(text)*h*((font_type==FONT_SMALLFNT)?0.75F:0.49F);
}

void font_render(float x, float y, float h, char* text) {
    int chars_x = 16;
    int chars_y = (font_type==FONT_SMALLFNT)?16:14;
    float size_ratio = (font_type==FONT_SMALLFNT)?0.75F:0.49F;

    float i = 0.0F;
    int k = 0;
    while(*text) {
        if(*text>31 || font_type==FONT_SMALLFNT) {
            float tx = ((*text-((font_type==FONT_FIXEDSYS)?32:0))%chars_x)/((float)chars_x);
            float ty = ((*text-((font_type==FONT_FIXEDSYS)?32:0))/chars_x)/((float)chars_y);

            //vertices
            font_vertex_buffer[k+0] = x+i;
            font_vertex_buffer[k+1] = y-h;

            font_vertex_buffer[k+2] = x+i+h*size_ratio;
            font_vertex_buffer[k+3] = y-h;

            font_vertex_buffer[k+4] = x+i+h*size_ratio;
            font_vertex_buffer[k+5] = y;

            font_vertex_buffer[k+6] = x+i;
            font_vertex_buffer[k+7] = y;


            //texture coordinates
            font_coords_buffer[k+0] = tx*8192.0F;
            font_coords_buffer[k+1] = (ty+1.0F/((float)chars_y))*8192.0F;

            font_coords_buffer[k+2] = (tx+1.0F/((float)chars_x))*8192.0F;
            font_coords_buffer[k+3] = (ty+1.0F/((float)chars_y))*8192.0F;

            font_coords_buffer[k+4] = (tx+1.0F/((float)chars_x))*8192.0F;
            font_coords_buffer[k+5] = ty*8192.0F;

            font_coords_buffer[k+6] = tx*8192.0F;
            font_coords_buffer[k+7] = ty*8192.0F;
            i += h*size_ratio;
            k += 8;
        }
        text++;
    }

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0F/8192.0F,1.0F/8192.0F,1.0F);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    switch(font_type) {
        case FONT_SMALLFNT:
            glBindTexture(GL_TEXTURE_2D,font_smallfnt.texture_id);
            break;
        case FONT_FIXEDSYS:
            glBindTexture(GL_TEXTURE_2D,font_fixedsys.texture_id);
            break;
    }
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2,GL_SHORT,0,font_vertex_buffer);
    glTexCoordPointer(2,GL_SHORT,0,font_coords_buffer);
    glDrawArrays(GL_QUADS,0,k/2);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void font_centered(float x, float y, float h, char* text) {
    font_render(x-font_length(h,text)/2.0F,y,h,text);
}
