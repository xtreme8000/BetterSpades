

unsigned char font_init() {
    unsigned char* image;
    unsigned width, height;

    int error = lodepng_decode32_file(&image,&width,&height,"fonts/FixedSys_Bold_36.png");
    if(error) {
        printf("Could not load font (%u): %s\n",error,lodepng_error_text(error));
        return 0;
    }
    
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1,&font_texture);
    glBindTexture(GL_TEXTURE_2D,font_texture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,image);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_TEXTURE_2D);

    free(image);

    font_vertex_buffer = malloc(512*8*sizeof(short));
    font_coords_buffer = malloc(512*8*sizeof(short));

    return 1;
}

float font_length(float h, char* text) {
    return strlen(text)*h*0.49F;
}

void font_render(float x, float y, float h, char* text) {
    float i = 0.0F;
    int k = 0;
    while(*text) {
        if(*text>31) {
            float tx = ((*text-32)%16)/16.0F;
            float ty = ((*text-32)/16)/14.0F;

            //vertices
            font_vertex_buffer[k+0] = x+i;
            font_vertex_buffer[k+1] = y-h;

            font_vertex_buffer[k+2] = x+i+h*0.49F;
            font_vertex_buffer[k+3] = y-h;

            font_vertex_buffer[k+4] = x+i+h*0.49F;
            font_vertex_buffer[k+5] = y;

            font_vertex_buffer[k+6] = x+i;
            font_vertex_buffer[k+7] = y;


            //texture coordinates
            font_coords_buffer[k+0] = tx*4096.0F;
            font_coords_buffer[k+1] = (ty+1.0F/14.0F)*4096.0F;

            font_coords_buffer[k+2] = (tx+1.0F/16.0F)*4096.0F;
            font_coords_buffer[k+3] = (ty+1.0F/14.0F)*4096.0F;

            font_coords_buffer[k+4] = (tx+1.0F/16.0F)*4096.0F;
            font_coords_buffer[k+5] = ty*4096.0F;

            font_coords_buffer[k+6] = tx*4096.0F;
            font_coords_buffer[k+7] = ty*4096.0F;
            i += h*0.49F;
            k += 8;
        }
        text++;
    }

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(1.0F/4096.0F,1.0F/4096.0F,1.0F);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,font_texture);
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
