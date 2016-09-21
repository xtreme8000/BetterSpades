typedef struct {
	float x,y,z;
	float vx,vy,vz;
	float size;
	long created;
	long fade;
	unsigned int color;
	boolean alive;
} Particle;

Particle* particles;
#define PARTICLES_MAX 8192

void particle_init();
void particle_update(float dt);
void particle_render();
void particle_create(float x, float y, float z, float velocity_x, float velocity_y, float velocity_z, int amount, float min_size, float max_size);