#define FONT_FIXEDSYS 0
#define FONT_SMALLFNT 1

int font_texture;
short* font_vertex_buffer;
short* font_coords_buffer;
int font_type = FONT_FIXEDSYS;

unsigned char font_init();
float font_length(float h, char* text);
void font_render(float x, float y, float h, char* text);
void font_centered(float x, float y, float h, char* text);
void font_select(char type);
