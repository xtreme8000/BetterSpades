#define FONT_FIXEDSYS 0
#define FONT_SMALLFNT 1

extern struct texture font_fixedsys;
extern struct texture font_smallfnt;

extern short* font_vertex_buffer;
extern short* font_coords_buffer;
extern int font_type;

unsigned char font_init(void);
float font_length(float h, char* text);
void font_render(float x, float y, float h, char* text);
void font_centered(float x, float y, float h, char* text);
void font_select(char type);
