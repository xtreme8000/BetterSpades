int texture_create(struct texture* t, char* filename) {
    int error = lodepng_decode32_file(&t->pixels,&t->width,&t->height,filename);
    if(error) {
        printf("Could not load texture (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }
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
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER,0.5F);
    glBindTexture(GL_TEXTURE_2D,t->texture_id);
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
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);
}

void texture_init() {
    texture_create(&texture_health,"png/health.png");
    texture_create(&texture_block,"png/block.png");
    texture_create(&texture_grenade,"png/grenade.png");
    texture_create(&texture_ammo_semi,"png/semiammo.png");
    texture_create(&texture_ammo_smg,"png/smgammo.png");
    texture_create(&texture_ammo_shotgun,"png/shotgunammo.png");

    int base[3][8]={{15,31,31,31, 0, 0, 0,31},
                    {15, 0,15,31,31,31, 0, 0},
                    {15, 0, 0, 0, 0,31,31,31}};

    int base_inc[8] = {32,64,0,0,0,0,0,0};
    unsigned int* pixels = malloc(64*64*sizeof(unsigned int));
    for(int k=0;k<64*64;k++) {
        pixels[k] = 0;
    }
    for(int y=0;y<8;y++) {
        for(int x=0;x<8;x++) {
            for(int ys=0;ys<6;ys++) {
                for(int xs=0;xs<6;xs++) {
                    if(x<4) {
                        pixels[(x*8+xs)+(y*8+ys)*64] = 0xFF000000 | rgb(base[0][y]+x*(base[0][y]*2+2)*(base[0][y]>0),base[1][y]+x*(base[1][y]*2+2)*(base[1][y]>0),base[2][y]+x*(base[2][y]*2+2)*(base[2][y]>0));
                    } else {
                        pixels[(x*8+xs)+(y*8+ys)*64] = 0xFF000000 | rgb(min(base[0][y]+x*(base[0][y]*2+2)*(base[0][y]>0)+((x-3)*64-33)*(!base[0][y]),255),
                                                                        min(base[1][y]+x*(base[1][y]*2+2)*(base[1][y]>0)+((x-3)*64-33)*(!base[1][y]),255),
                                                                        min(base[2][y]+x*(base[2][y]*2+2)*(base[2][y]>0)+((x-3)*64-33)*(!base[2][y]),255));
                    }
                }
            }
        }
    }
    texture_create_buffer(&texture_color_selection,64,64,(unsigned char*)pixels);
}
