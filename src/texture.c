#include "common.h"

#include "lodepng/lodepng.c"

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

struct texture texture_player;
struct texture texture_medical;
struct texture texture_intel;
struct texture texture_command;
struct texture texture_tracer;

int texture_create(struct texture* t, char* filename) {
    int error = lodepng_decode32_file(&t->pixels,&t->width,&t->height,filename);
    if(error) {
        printf("Could not load texture (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }

    texture_resize_pow2(t);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1,&t->texture_id);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,t->pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);
}

int texture_create_buffer(struct texture* t, int width, int height, unsigned char* buff) {
    t->width = width;
    t->height = height;
    t->pixels = buff;
    texture_resize_pow2(t);

    glEnable(GL_TEXTURE_2D);
    glGenTextures(1,&t->texture_id);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t->width,t->height,0,GL_RGBA,GL_UNSIGNED_BYTE,t->pixels);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
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

    printf("original: %i:%i now: %i:%i\n",t->width,t->height,w,h);

    unsigned int* pixels_new = malloc(w*h*sizeof(unsigned int));
    for(int y=0;y<h;y++) {
        for(int x=0;x<w;x++) {
            pixels_new[x+y*w] = ((unsigned int*)t->pixels)[x*t->width/w+y*t->height/h*t->width];
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

void texture_init() {
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

    texture_create(&texture_player,"png/player.png");
    texture_create(&texture_medical,"png/medical.png");
    texture_create(&texture_intel,"png/intel.png");
    texture_create(&texture_command,"png/command.png");
    texture_create(&texture_tracer,"png/tracer.png");


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
}
