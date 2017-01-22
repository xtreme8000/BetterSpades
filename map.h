unsigned long long* map_colors = 0;
int map_size_x = 0;
int map_size_y = 0;
int map_size_z = 0;

#define MAX_VOXEL_CHECKS 8192
int map_checked_voxels_x[MAX_VOXEL_CHECKS] = {0};
int map_checked_voxels_y[MAX_VOXEL_CHECKS] = {0};
int map_checked_voxels_z[MAX_VOXEL_CHECKS] = {0};
int map_checked_voxels_index = 0;
boolean map_ground_connected_result = false;

void map_ground_connected_sub(int x, int y, int z, int depth);
boolean map_ground_connected(int x, int y, int z);
void map_apply_gravity();
unsigned long long map_get(int x, int y, int z);
void map_set(int x, int y, int z, unsigned long long color);
void map_vxl_setgeom(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_setcolor(int x, int y, int z, unsigned int t, unsigned long long* map);
void map_vxl_load_s(unsigned char* v);
void map_vxl_load(unsigned char* v, unsigned long long* map);
