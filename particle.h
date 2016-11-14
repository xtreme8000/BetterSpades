typedef struct {
	float x,y,z;
	float vx,vy,vz;
	float size;
	long created;
	long fade;
	unsigned int color;
	boolean alive;
} Particle;

Particle* particles = {0};
float* particles_vertices = 0;
unsigned char* particles_colors = 0;
#define PARTICLES_MAX 8192

int vertex_index = 0;

void particle_init();
void particle_update(float dt);
int particle_render();
void particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount, float min_size, float max_size);