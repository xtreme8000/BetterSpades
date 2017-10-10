

unsigned char font_init() {
    unsigned char* image;
    unsigned char* image2;
    unsigned int width, height, width2, height2;
    int error;

    error = lodepng_decode32_file(&image,&width,&height,"fonts/FixedSys_Bold_36.png");
    if(error) {
        printf("Could not load font (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }

    error = lodepng_decode32_file(&image2,&width2,&height2,"fonts/smallfnt68.png");
    if(error) {
        printf("Could not load font (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }

    glEnable(GL_TEXTURE_2D);
    glGenTextures(2,&font_texture);

    glBindTexture(GL_TEXTURE_2D,font_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D,font_texture+1);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width2,height2,0,GL_RGBA,GL_UNSIGNED_BYTE,image2);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);


    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);

    free(image);
    free(image2);

    font_vertex_buffer = malloc(512*8*sizeof(short));
    font_coords_buffer = malloc(512*8*sizeof(short));

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
            font_coords_buffer[k+0] = tx*4096.0F;
            font_coords_buffer[k+1] = (ty+1.0F/((float)chars_y))*4096.0F;

            font_coords_buffer[k+2] = (tx+1.0F/((float)chars_x))*4096.0F;
            font_coords_buffer[k+3] = (ty+1.0F/((float)chars_y))*4096.0F;

            font_coords_buffer[k+4] = (tx+1.0F/((float)chars_x))*4096.0F;
            font_coords_buffer[k+5] = ty*4096.0F;

            font_coords_buffer[k+6] = tx*4096.0F;
            font_coords_buffer[k+7] = ty*4096.0F;
            i += h*size_ratio;
            k += 8;
        }
        text++;
    }

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0F/4096.0F,1.0F/4096.0F,1.0F);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,font_texture+((font_type==FONT_SMALLFNT)?1:0));
    glAlphaFunc(GL_GREATER,0.5F);
    glEnable(GL_ALPHA_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(2,GL_SHORT,0,font_vertex_buffer);
    glTexCoordPointer(2,GL_SHORT,0,font_coords_buffer);
    glDrawArrays(GL_QUADS,0,k/2);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
}

void font_centered(float x, float y, float h, char* text) {
    font_render(x-font_length(h,text)/2.0F,y,h,text);
}
