unsigned int* map_colors;
int map_size_x = 512;
int map_size_y = 64;
int map_size_z = 512;

unsigned int map_get(int x, int y, int z);
void map_set(int x, int y, int z, unsigned int color);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned int* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned int* map);
void map_vxl_load(unsigned char* v, unsigned int* map);