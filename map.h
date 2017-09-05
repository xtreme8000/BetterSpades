unsigned long long* map_colors;
int map_size_x = 512;
int map_size_y = 64;
int map_size_z = 512;

struct Point {
	int x,y,z;
};

void map_update_physics(int x, int y, int z);
void map_ground_connected_sub(int x, int y, int z, int depth);
boolean map_ground_connected(int x, int y, int z);
void map_apply_gravity();
unsigned long long map_get(int x, int y, int z);
void map_set(int x, int y, int z, unsigned long long color);
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_load(unsigned char* v, unsigned long long* map);
