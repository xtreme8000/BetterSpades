void particle_init() {
	particles = malloc(sizeof(Particle)*PARTICLES_MAX);
	memset(particles,0,sizeof(Particle)*PARTICLES_MAX);
}

void particle_update(float dt) {
	AABB a;
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(particles[k].alive) {
			if(glutGet(GLUT_ELAPSED_TIME)-particles[k].created>=60000) {
				particles[k].alive = false;
			} else {
				a.min_x = a.min_y = a.min_z = 0.0F;
				float size = particles[k].size;
				if(particles[k].fade) {
					size *= 1.0F-((float)(glutGet(GLUT_ELAPSED_TIME)-particles[k].fade)/2000.0F);
				}
				
				a.max_x = a.max_y = a.max_z = size;
				
				float acc_y = -9.81F*(1.0F/0.64F)*dt;
				aabb_set_center(&a,particles[k].x,particles[k].y+acc_y*dt,particles[k].z);
				if(!aabb_intersection_terrain(&a)) {
					particles[k].vy += acc_y;
				}
				float movement_x = particles[k].vx*dt;
				float movement_y = particles[k].vy*dt;
				float movement_z = particles[k].vz*dt;
				boolean on_ground = false;
				aabb_set_center(&a,particles[k].x+movement_x,particles[k].y,particles[k].z);
				if(aabb_intersection_terrain(&a)) {
					movement_x = 0.0F;
					particles[k].vx = -particles[k].vx*0.6F;
					on_ground = true;
				}
				aabb_set_center(&a,particles[k].x+movement_x,particles[k].y+movement_y,particles[k].z);
				if(aabb_intersection_terrain(&a)) {
					movement_y = 0.0F;
					particles[k].vy = -particles[k].vy*0.6F;
					on_ground = true;
				}
				aabb_set_center(&a,particles[k].x+movement_x,particles[k].y+movement_y,particles[k].z+movement_z);
				if(aabb_intersection_terrain(&a)) {
					movement_z = 0.0F;
					particles[k].vz = -particles[k].vz*0.6F;
					on_ground = true;
				}
				//air and ground friction
				if(on_ground) {
					particles[k].vx *= pow(0.1F,dt);
					particles[k].vy *= pow(0.1F,dt);
					particles[k].vz *= pow(0.1F,dt);
					boolean can_fade = true;
					if(abs(particles[k].vx)<0.1F) {
						particles[k].vx = 0.0F;
					} else {
						can_fade = false;
					}
					if(abs(particles[k].vy)<0.1F) {
						particles[k].vy = 0.0F;
					} else {
						can_fade = false;
					}
					if(abs(particles[k].vz)<0.1F) {
						particles[k].vz = 0.0F;
					} else {
						can_fade = false;
					}
					if(can_fade && particles[k].fade==0) {
						particles[k].fade = glutGet(GLUT_ELAPSED_TIME);
					}
				} else {
					particles[k].vx *= pow(0.8F,dt);
					particles[k].vy *= pow(0.8F,dt);
					particles[k].vz *= pow(0.8F,dt); 
				}
				particles[k].x += movement_x;
				particles[k].y += movement_y;
				particles[k].z += movement_z;
			}
		}
	}
}

void particle_render() {
	AABB a;
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(particles[k].alive) {
			a.min_x = a.min_y = a.min_z = 0.0F;
			a.max_x = a.max_y = a.max_z = particles[k].size;
			aabb_set_center(&a,particles[k].x,particles[k].y,particles[k].z);
			aabb_render(&a);
		}
	}
}

void particle_create(float x, float y, float z, float velocity_x, float velocity_y, float velocity_z, int amount, float min_size, float max_size) {
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(!particles[k].alive) {
			particles[k].size = ((float)rand()/(float)RAND_MAX)*(max_size-min_size)+min_size;
			particles[k].x = x;
			particles[k].y = y;
			particles[k].z = z;
			particles[k].vx = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F)*velocity_x;
			particles[k].vy = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F)*velocity_y;
			particles[k].vz = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F)*velocity_z;
			particles[k].created = glutGet(GLUT_ELAPSED_TIME);
			particles[k].fade = 0;
			particles[k].alive = true;
			amount--;
			if(amount==0) {
				return;
			}
		}
	}
}