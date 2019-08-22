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
#include "stb_truetype.h"

short* font_vertex_buffer;
short* font_coords_buffer;
int font_type = FONT_FIXEDSYS;

struct font_backed {
    stbtt_bakedchar* cdata;
    int type;
    int texture_id;
    int w, h;
    float size;
};

struct list font_backed_fonts;

unsigned char font_init() {
    font_vertex_buffer = malloc(512*8*sizeof(short));
    CHECK_ALLOCATION_ERROR(font_vertex_buffer)
    font_coords_buffer = malloc(512*8*sizeof(short));
    CHECK_ALLOCATION_ERROR(font_coords_buffer)

    list_create(&font_backed_fonts,sizeof(struct font_backed));

    return 1;
}

void font_select(char type) {
    font_type = type;
}

static struct font_backed* font_find(float h) {
    if(font_type==FONT_SMALLFNT)
        h *= 1.5F;

    for(int k=0;k<list_size(&font_backed_fonts);k++) {
        struct font_backed* b = (struct font_backed*)list_get(&font_backed_fonts,k);
        if(b->size==h && b->type==font_type)
            return (struct font_backed*)list_get(&font_backed_fonts,k);
    }

    struct font_backed f;
    f.w = 64;
    f.h = 64;
    f.size = h;
    f.type = font_type;
    f.cdata = malloc(224*sizeof(stbtt_bakedchar));
    CHECK_ALLOCATION_ERROR(f.cdata)

    void* temp_bitmap = NULL;

    int max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size);

    void* file;
    switch(font_type) {
        case FONT_FIXEDSYS:
            file = file_load("fonts/Fixedsys.ttf");
            break;
        case FONT_SMALLFNT:
            file = file_load("fonts/Terminal.ttf");
            break;
		default:
			free(f.cdata);
			return NULL;
    }

    while(1) {
        temp_bitmap = realloc(temp_bitmap,f.w*f.h);
        CHECK_ALLOCATION_ERROR(temp_bitmap)
        int res = 0;
        res = stbtt_BakeFontBitmap(file,0,f.size,temp_bitmap,f.w,f.h,31,224,f.cdata);
        if(res>0 || (f.w==max_size && f.h==max_size))
            break;
        if(f.h>f.w)
            f.w *= 2;
        else
            f.h *= 2;
    }

    log_info("font texsize: %i:%ipx [size %f] type: %i",f.w,f.h,f.size,f.type);

    glGenTextures(1,&f.texture_id);
    glBindTexture(GL_TEXTURE_2D,f.texture_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_ALPHA,f.w,f.h,0,GL_ALPHA,GL_UNSIGNED_BYTE,temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D,0);

    free(temp_bitmap);
    free(file);

    return list_add(&font_backed_fonts,&f);
}

float font_length(float h, char* text) {
    struct font_backed* font = font_find(h);

	if(!font)
		return 0.0F;

    stbtt_aligned_quad q;
    float y = h*0.75F;
    float x = 0.0F;
    for(int k=0;k<strlen(text);k++) {
        if(text[k]>30) {
            stbtt_GetBakedQuad(font->cdata,font->w,font->h,text[k]-31,&x,&y,&q,1);
        }
    }
    return x+h*0.125F;
}

void font_reset() {
    for(int k=0;k<list_size(&font_backed_fonts);k++) {
        struct font_backed* f = (struct font_backed*)list_get(&font_backed_fonts,k);
        glDeleteTextures(1,&f->texture_id);
        free(f->cdata);
    }
    list_clear(&font_backed_fonts);
}

void font_render(float x, float y, float h, char* text) {

    struct font_backed* font = font_find(h);

	if(!font)
		return;

    int chars_x = 16;
    int chars_y = (font_type==FONT_SMALLFNT)?16:14;
    float size_ratio = (font_type==FONT_SMALLFNT)?0.75F:0.49F;

    float i = 0.0F;
    int k = 0;
    float y2 = h*0.75F;
    while(*text) {
        if(*text>31) {
            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->cdata,font->w,font->h,*text-31,&x,&y2,&q,1);
            font_coords_buffer[k+0] = q.s0*8192.0F;
            font_coords_buffer[k+1] = q.t1*8192.0F;
            font_coords_buffer[k+2] = q.s1*8192.0F;
            font_coords_buffer[k+3] = q.t1*8192.0F;
            font_coords_buffer[k+4] = q.s1*8192.0F;
            font_coords_buffer[k+5] = q.t0*8192.0F;

            font_coords_buffer[k+6] = q.s0*8192.0F;
            font_coords_buffer[k+7] = q.t1*8192.0F;
            font_coords_buffer[k+8] = q.s1*8192.0F;
            font_coords_buffer[k+9] = q.t0*8192.0F;
            font_coords_buffer[k+10] = q.s0*8192.0F;
            font_coords_buffer[k+11] = q.t0*8192.0F;

            font_vertex_buffer[k+0] = q.x0;
            font_vertex_buffer[k+1] = -q.y1+y;
            font_vertex_buffer[k+2] = q.x1;
            font_vertex_buffer[k+3] = -q.y1+y;
            font_vertex_buffer[k+4] = q.x1;
            font_vertex_buffer[k+5] = -q.y0+y;

            font_vertex_buffer[k+6] = q.x0;
            font_vertex_buffer[k+7] = -q.y1+y;
            font_vertex_buffer[k+8] = q.x1;
            font_vertex_buffer[k+9] = -q.y0+y;
            font_vertex_buffer[k+10] = q.x0;
            font_vertex_buffer[k+11] = -q.y0+y;
            k += 12;
        } else {
            x += h*0.49F;
        }
        text++;
    }

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0F/8192.0F,1.0F/8192.0F,1.0F);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,font->texture_id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2,GL_SHORT,0,font_vertex_buffer);
    glTexCoordPointer(2,GL_SHORT,0,font_coords_buffer);
    glDrawArrays(GL_TRIANGLES,0,k/2);
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
