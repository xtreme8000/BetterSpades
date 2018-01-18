extern unsigned long long* map_colors;
extern int map_size_x;
extern int map_size_y;
extern int map_size_z;

extern float fog_color[4];
extern float map_sun[4];

extern unsigned char* map_minimap;

extern struct DamagedVoxel {
	int x,y,z;
	char damage;
	float timer;
} map_damaged_voxels[8];

struct Point {
	int x,y,z;
};

int map_damage(int x, int y, int z, int damage);
void map_damaged_voxels_render();
void map_update_physics(int x, int y, int z);
unsigned long long map_get(int x, int y, int z);
unsigned long long map_get_unblocked(int x, int y, int z);
void map_set(int x, int y, int z, unsigned long long color);
int map_cube_line(int x1, int y1, int z1, int x2, int y2, int z2, struct Point* cube_array);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_load(unsigned char* v, unsigned long long* map);
void map_collapsing_render(float dt);
