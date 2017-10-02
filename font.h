int font_texture;
short* font_vertex_buffer;
short* font_coords_buffer;

unsigned char font_init();
float font_length(float h, char* text);
void font_render(float x, float y, float h, char* text);
void font_centered(float x, float y, float h, char* text);
