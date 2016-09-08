typedef struct {
	float x,y,z;
	float vx,vy,vz;
	float size;
	long created;
	unsigned int color;
	boolean alive;
} Particle;

Particle* particles;
#define PARTICLES_MAX 8192

void particle_init();
void particle_update(float dt);
void particle_render();
int particle_create(float x, float y, float z);