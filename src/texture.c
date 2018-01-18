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

#include "lodepng/lodepng.c"

struct texture texture_splash;
struct texture texture_minimap;
struct texture texture_gradient;

struct texture texture_health;
struct texture texture_block;
struct texture texture_grenade;
struct texture texture_ammo_semi;
struct texture texture_ammo_smg;
struct texture texture_ammo_shotgun;

struct texture texture_color_selection;

struct texture texture_zoom_semi;
struct texture texture_zoom_smg;
struct texture texture_zoom_shotgun;

struct texture texture_white;
struct texture texture_target;
struct texture texture_indicator;

struct texture texture_player;
struct texture texture_medical;
struct texture texture_intel;
struct texture texture_command;
struct texture texture_tracer;

struct texture texture_ui_wait;
struct texture texture_ui_join;
struct texture texture_ui_reload;
struct texture texture_ui_bg;
struct texture texture_ui_input;

void texture_filter(struct texture* t, int filter) {
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    switch(filter) {
        case TEXTURE_FILTER_NEAREST:
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            break;
        case TEXTURE_FILTER_LINEAR:
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            break;
    }
    glBindTexture(GL_TEXTURE_2D,0);
}

int texture_create(struct texture* t, char* filename) {
    int error = lodepng_decode32_file(&t->pixels,&t->width,&t->height,filename);
    if(error) {
        printf("Could not load texture (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }

    texture_resize_pow2(t);

    glPixelStorei(GL_UNPACK_ROW_LENGTH,t->width);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glGenTextures(1,&t->texture_id);
    glxcheckErrors(__FILE__,__LINE__);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,t->pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
}

int texture_create_buffer(struct texture* t, int width, int height, unsigned char* buff) {
    if(t->pixels==NULL) {
        glGenTextures(1,&t->texture_id);
    }
    t->width = width;
    t->height = height;
    t->pixels = buff;
    texture_resize_pow2(t);

    glPixelStorei(GL_UNPACK_ROW_LENGTH,t->width);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,t->pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
}

void texture_draw_sector(struct texture* t, float x, float y, float w, float h, float u, float v, float us, float vs) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);

    glBegin(GL_QUADS);
    glTexCoord2f(u,v);
    glVertex2f(x,y);
    glTexCoord2f(u,v+vs);
    glVertex2f(x,y-h);
    glTexCoord2f(u+us,v+vs);
    glVertex2f(x+w,y-h);
    glTexCoord2f(u+us,v);
    glVertex2f(x+w,y);
    glEnd();

    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw(struct texture* t, float x, float y, float w, float h) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    texture_draw_empty(x,y,w,h);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw_rotated(struct texture* t, float x, float y, float w, float h, float angle) {
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    texture_draw_empty_rotated(x,y,w,h,angle);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void texture_draw_empty(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glTexCoord2f(0.0F,0.0F);
    glVertex2f(x,y);
    glTexCoord2f(0.0F,1.0F);
    glVertex2f(x,y-h);
    glTexCoord2f(1.0F,1.0F);
    glVertex2f(x+w,y-h);
    glTexCoord2f(1.0F,0.0F);
    glVertex2f(x+w,y);
    glEnd();
}

#define texture_emit_rotated(tx,ty,x,y,a) glVertex2f(cos(a)*x-sin(a)*y+tx,sin(a)*x+cos(a)*y+ty)

void texture_draw_empty_rotated(float x, float y, float w, float h, float angle) {
    glBegin(GL_QUADS);
    glTexCoord2f(0.0F,0.0F);
    texture_emit_rotated(x,y,-w/2,h/2,angle);
    glTexCoord2f(0.0F,1.0F);
    texture_emit_rotated(x,y,-w/2,-h/2,angle);
    glTexCoord2f(1.0F,1.0F);
    texture_emit_rotated(x,y,w/2,-h/2,angle);
    glTexCoord2f(1.0F,0.0F);
    texture_emit_rotated(x,y,w/2,h/2,angle);
    glEnd();
}

void texture_resize_pow2(struct texture* t) {
    if(strstr(glGetString(GL_EXTENSIONS),"ARB_texture_non_power_of_two")!=NULL) {
        return;
    }

    int w = 1, h = 1;
    while(w<t->width) {
        w += w;
    }
    while(h<t->height) {
        h += h;
    }

    if(t->width==w && t->height==h) {
        return;
    }

    printf("original: %i:%i now: %i:%i\n",t->width,t->height,w,h);

    unsigned int* pixels_new = malloc(w*h*sizeof(unsigned int));
    for(int y=0;y<h;y++) {
        for(int x=0;x<w;x++) {
            float px = (float)x/(float)w*(float)t->width;
            float py = (float)y/(float)h*(float)t->height;
            float u = px-(int)px;
            float v = py-(int)py;
            unsigned int aa = ((unsigned int*)t->pixels)[(int)px+(int)py*t->width];
            unsigned int ba = ((unsigned int*)t->pixels)[min((int)px+(int)py*t->width+1,t->width*t->height)];
            unsigned int ab = ((unsigned int*)t->pixels)[min((int)px+(int)py*t->width+t->width,t->width*t->height)];
            unsigned int bb = ((unsigned int*)t->pixels)[min((int)px+(int)py*t->width+1+t->width,t->width*t->height)];
            pixels_new[x+y*w] = 0;
            pixels_new[x+y*w] |= (int)((1.0F-v)*u*red(ba)+(1.0F-v)*(1.0F-u)*red(aa)+v*u*red(bb)+v*(1.0F-u)*red(ab));
            pixels_new[x+y*w] |= (int)((1.0F-v)*u*green(ba)+(1.0F-v)*(1.0F-u)*green(aa)+v*u*green(bb)+v*(1.0F-u)*green(ab))<<8;
            pixels_new[x+y*w] |= (int)((1.0F-v)*u*blue(ba)+(1.0F-v)*(1.0F-u)*blue(aa)+v*u*blue(bb)+v*(1.0F-u)*blue(ab))<<16;
            pixels_new[x+y*w] |= (int)((1.0F-v)*u*alpha(ba)+(1.0F-v)*(1.0F-u)*alpha(aa)+v*u*alpha(bb)+v*(1.0F-u)*alpha(ab))<<24;
        }
    }

    t->width = w;
    t->height = h;
    free(t->pixels);
    t->pixels = (unsigned char*)pixels_new;
}

unsigned int texture_block_color(int x, int y) {
    int base[3][8]={{15,31,31,31, 0, 0, 0,31},
                    {15, 0,15,31,31,31, 0, 0},
                    {15, 0, 0, 0, 0,31,31,31}};

    if(x<4) {
        return rgb(base[0][y]+x*(base[0][y]*2+2)*(base[0][y]>0),
                   base[1][y]+x*(base[1][y]*2+2)*(base[1][y]>0),
                   base[2][y]+x*(base[2][y]*2+2)*(base[2][y]>0));
    } else {
        return rgb(min(base[0][y]+x*(base[0][y]*2+2)*(base[0][y]>0)+((x-3)*64-33)*(!base[0][y]),255),
                   min(base[1][y]+x*(base[1][y]*2+2)*(base[1][y]>0)+((x-3)*64-33)*(!base[1][y]),255),
                   min(base[2][y]+x*(base[2][y]*2+2)*(base[2][y]>0)+((x-3)*64-33)*(!base[2][y]),255));
    }
}

void texture_gradient_fog(unsigned int* gradient) {
    for(int y=0;y<256;y++) {
        for(int x=0;x<256;x++) {
            int d = min(sqrt(distance2D(128,128,x,y))/128.0F*255.0F,255);
            gradient[x+y*256] = (d<<24)|rgb((int)(fog_color[0]*255.0F),(int)(fog_color[1]*255.0F),(int)(fog_color[2]*255.0F));
        }
    }
}

void texture_init() {
    texture_create(&texture_splash,"png/splash.png");

    texture_create(&texture_health,"png/health.png");
    texture_create(&texture_block,"png/block.png");
    texture_create(&texture_grenade,"png/grenade.png");
    texture_create(&texture_ammo_semi,"png/semiammo.png");
    texture_create(&texture_ammo_smg,"png/smgammo.png");
    texture_create(&texture_ammo_shotgun,"png/shotgunammo.png");

    texture_create(&texture_zoom_semi,"png/semi.png");
    texture_create(&texture_zoom_smg,"png/smg.png");
    texture_create(&texture_zoom_shotgun,"png/shotgun.png");

    texture_create(&texture_white,"png/white.png");
    texture_create(&texture_target,"png/target.png");
    texture_create(&texture_indicator,"png/indicator.png");

    texture_create(&texture_player,"png/player.png");
    texture_create(&texture_medical,"png/medical.png");
    texture_create(&texture_intel,"png/intel.png");
    texture_create(&texture_command,"png/command.png");
    texture_create(&texture_tracer,"png/tracer.png");


    texture_create(&texture_ui_wait,"png/ui/wait.png");
    texture_create(&texture_ui_join,"png/ui/join.png");
    texture_create(&texture_ui_reload,"png/ui/reload.png");
    texture_create(&texture_ui_bg,"png/ui/bg.png");
    texture_create(&texture_ui_input,"png/ui/input.png");


    unsigned int* pixels = malloc(64*64*sizeof(unsigned int));
    memset(pixels,0,64*64*sizeof(unsigned int));
    for(int y=0;y<8;y++) {
        for(int x=0;x<8;x++) {
            for(int ys=0;ys<6;ys++) {
                for(int xs=0;xs<6;xs++) {
                    pixels[(x*8+xs)+(y*8+ys)*64] = 0xFF000000 | texture_block_color(x,y);
                }
            }
        }
    }
    texture_create_buffer(&texture_color_selection,64,64,(unsigned char*)pixels);

    texture_create_buffer(&texture_minimap,map_size_x,map_size_z,map_minimap);

    unsigned int* gradient = malloc(256*256*sizeof(unsigned int));
    texture_gradient_fog(gradient);
    texture_create_buffer(&texture_gradient,256,256,(unsigned char*)gradient);
}
