typedef struct {
	unsigned short xsiz, ysiz, zsiz;
	float xpiv, ypiv, zpiv;
	unsigned char has_display_list;
	int display_list;
	unsigned char* display_list_colors;
	short* display_list_vertices;
	unsigned int* color;
} kv6_t;

kv6_t tent;
kv6_t gun;

void kv6_render(kv6_t* kv6, unsigned char red, unsigned char green, unsigned char blue);
kv6_t kv6_load(unsigned char* bytes);