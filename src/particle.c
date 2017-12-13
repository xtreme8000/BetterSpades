#include "common.h"

Particle* particles;
float* particles_vertices;
unsigned char* particles_colors;

void particle_init() {
	particles = malloc(sizeof(Particle)*PARTICLES_MAX);
	memset(particles,0,sizeof(Particle)*PARTICLES_MAX);
	particles_vertices = malloc(sizeof(float)*72*PARTICLES_MAX);
	particles_colors = malloc(sizeof(unsigned char)*72*PARTICLES_MAX);
}

void particle_update(float dt) {
	AABB a;
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(particles[k].alive) {
			if(!particles[k].fade /*&& glfwGetTime()-particles[k].created>=30.0F*/) {
				particles[k].fade = glfwGetTime();
			}

			a.min_x = a.min_y = a.min_z = 0.0F;
			float size = particles[k].size;
			if(particles[k].fade) {
				size *= 1.0F-((float)(glfwGetTime()-particles[k].fade)/2.0F);
			}
			if(size<0.01F) {
				particles[k].alive = false;
			} else {
				a.max_x = a.max_y = a.max_z = size;

				float acc_y = -32.0F*dt;
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
					if(can_fade && !particles[k].fade) {
						particles[k].fade = glfwGetTime();
					}
				} else {
					particles[k].vx *= pow(0.4F,dt);
					particles[k].vy *= pow(0.4F,dt);
					particles[k].vz *= pow(0.4F,dt);
				}
				particles[k].x += movement_x;
				particles[k].y += movement_y;
				particles[k].z += movement_z;
			}
		}
	}
}

int vertex_index;

int particle_render() {
	int color_index = 0;
	vertex_index = 0;
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(particles[k].alive) {
			float size = particles[k].size/2.0F;
			if(particles[k].fade) {
				size *= 1.0F-((float)(glfwGetTime()-particles[k].fade)/2.0F);
			}

			if(particles[k].type==255) {
				for(int i=0;i<24;i++) {
					particles_colors[color_index++] = particles[k].color&0xFF;
					particles_colors[color_index++] = (particles[k].color>>8)&0xFF;
					particles_colors[color_index++] = (particles[k].color>>16)&0xFF;
				}

				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;

				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;

				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;

				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;

				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z-size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z-size;

				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y-size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x+size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
				particles_vertices[vertex_index++] = particles[k].x-size;
				particles_vertices[vertex_index++] = particles[k].y+size;
				particles_vertices[vertex_index++] = particles[k].z+size;
			} else {
				matrix_push();
				matrix_identity();
				matrix_translate(particles[k].x,particles[k].y,particles[k].z);
				matrix_pointAt(particles[k].ox,particles[k].oy,particles[k].oz);
				matrix_rotate(90.0F,0.0F,1.0F,0.0F);
				matrix_upload();
				kv6_render((struct kv6_t*[]){&model_semi_casing,&model_smg_casing,&model_shotgun_casing}[particles[k].type],TEAM_1);
				matrix_pop();
			}
		}
	}

	if(vertex_index>0) {
		matrix_upload();
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,particles_vertices);
		glColorPointer(3,GL_UNSIGNED_BYTE,0,particles_colors);
		glDrawArrays(GL_QUADS,0,vertex_index/3);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	return vertex_index/72;
}

void particle_create_casing(struct Player* p) {
	for(int k=0;k<PARTICLES_MAX;k++) {
        if(!particles[k].alive) {
            particles[k].size = 0.1F;
            particles[k].x = p->gun_pos.x;
            particles[k].y = p->gun_pos.y;
            particles[k].z = p->gun_pos.z;
            particles[k].ox = p->orientation.x;
            particles[k].oy = p->orientation.y;
            particles[k].oz = p->orientation.z;
            particles[k].vx = p->casing_dir.x*3.5F;
            particles[k].vy = p->casing_dir.y*3.5F;
            particles[k].vz = p->casing_dir.z*3.5F;
            particles[k].created = glfwGetTime();
            particles[k].fade = 0;
            particles[k].type = players[local_player_id].weapon;
            particles[k].color = 0x00FFFF;
            particles[k].alive = true;
            break;
        }
    }
}

void particle_create(unsigned int color, float x, float y, float z, float velocity, float velocity_y, int amount, float min_size, float max_size) {
	for(int k=0;k<PARTICLES_MAX;k++) {
		if(!particles[k].alive) {
			particles[k].size = ((float)rand()/(float)RAND_MAX)*(max_size-min_size)+min_size;
			particles[k].x = x;
			particles[k].y = y;
			particles[k].z = z;
			particles[k].vx = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F);
			particles[k].vy = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F);
			particles[k].vz = (((float)rand()/(float)RAND_MAX)*2.0F-1.0F);
			float len = sqrt(particles[k].vx*particles[k].vx+particles[k].vy*particles[k].vy+particles[k].vz*particles[k].vz);
			particles[k].vx = (particles[k].vx/len)*velocity;
			particles[k].vy = (particles[k].vy/len)*velocity*velocity_y;
			particles[k].vz = (particles[k].vz/len)*velocity;
			particles[k].created = glfwGetTime();
			particles[k].fade = 0;
			particles[k].color = color;
			particles[k].type = 255;
			particles[k].alive = true;
			amount--;
			if(amount==0) {
				return;
			}
		}
	}
}
