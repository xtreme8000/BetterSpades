typedef struct {
	float min_x, min_y, min_z;
	float max_x, max_y, max_z;
} AABB;

typedef struct {
	struct {
		float x,y,z;
	} origin;
	struct {
		float x,y,z;
	} direction;
} Ray;

unsigned char aabb_intersection(AABB* a, AABB* b);
char aabb_intersection_ray(AABB* a, Ray* r);
unsigned char aabb_intersection_terrain(AABB* a);
void aabb_set_size(AABB* a, float x, float y, float z);
void aabb_set_center(AABB* a, float x, float y, float z);
void aabb_render(AABB* a);
