#include "common.h"
#include "model_normals.h"

struct kv6_t model_playerdead;
struct kv6_t model_playerhead;
struct kv6_t model_playertorso;
struct kv6_t model_playertorsoc;
struct kv6_t model_playerarms;
struct kv6_t model_playerleg;
struct kv6_t model_playerlegc;
struct kv6_t model_intel;
struct kv6_t model_tent;

struct kv6_t model_semi;
struct kv6_t model_smg;
struct kv6_t model_shotgun;
struct kv6_t model_spade;
struct kv6_t model_block;
struct kv6_t model_grenade;

struct kv6_t model_semi_tracer;
struct kv6_t model_smg_tracer;
struct kv6_t model_shotgun_tracer;

void kv6_init() {
	kv6_load(&model_playerdead,file_load("kv6/playerdead.kv6"),0.1F);
	kv6_load(&model_playerhead,file_load("kv6/playerhead.kv6"),0.1F);
	kv6_load(&model_playertorso,file_load("kv6/playertorso.kv6"),0.1F);
	kv6_load(&model_playertorsoc,file_load("kv6/playertorsoc.kv6"),0.1F);
	kv6_load(&model_playerarms,file_load("kv6/playerarms.kv6"),0.1F);
	kv6_load(&model_playerleg,file_load("kv6/playerleg.kv6"),0.1F);
	kv6_load(&model_playerlegc,file_load("kv6/playerlegc.kv6"),0.1F);

	kv6_load(&model_intel,file_load("kv6/intel.kv6"),0.278F);
	kv6_load(&model_tent,file_load("kv6/cp.kv6"),0.278F);

	kv6_load(&model_semi,file_load("kv6/semi.kv6"),0.05F);
	kv6_load(&model_smg,file_load("kv6/smg.kv6"),0.05F);
	kv6_load(&model_shotgun,file_load("kv6/shotgun.kv6"),0.05F);
	kv6_load(&model_spade,file_load("kv6/spade.kv6"),0.05F);
	kv6_load(&model_block,file_load("kv6/block.kv6"),0.05F);
	model_block.colorize = 1;
	kv6_load(&model_grenade,file_load("kv6/grenade.kv6"),0.05F);

	kv6_load(&model_semi_tracer,file_load("kv6/semitracer.kv6"),0.05F);
	kv6_load(&model_smg_tracer,file_load("kv6/smgtracer.kv6"),0.05F);
	kv6_load(&model_shotgun_tracer,file_load("kv6/shotguntracer.kv6"),0.05F);
}

void kv6_rebuild_all() {
	kv6_rebuild(&model_playerdead);
	kv6_rebuild(&model_playerhead);
	kv6_rebuild(&model_playertorso);
	kv6_rebuild(&model_playertorsoc);
	kv6_rebuild(&model_playerarms);
	kv6_rebuild(&model_playerleg);
	kv6_rebuild(&model_playerlegc);
	kv6_rebuild(&model_intel);
	kv6_rebuild(&model_tent);
}

void kv6_load(struct kv6_t* kv6, unsigned char* bytes, float scale) {
	kv6->colorize = 0;
	kv6->has_display_list = 0;
	kv6->scale = scale;
	int index = 0;
	if(buffer_read32(bytes,index)==0x6C78764B) { //"Kvxl"
		index += 4;
		kv6->xsiz = buffer_read32(bytes,index);
		index += 4;
		kv6->ysiz = buffer_read32(bytes,index);
		index += 4;
		kv6->zsiz = buffer_read32(bytes,index);
		index += 4;

		kv6->xpiv = buffer_readf(bytes,index);
		index += 4;
		kv6->ypiv = buffer_readf(bytes,index);
		index += 4;
		kv6->zpiv = kv6->zsiz-buffer_readf(bytes,index);
		index += 4;

		int blklen = buffer_read32(bytes,index);
		index += 4;
		unsigned int* blkdata_color = malloc(blklen*4);
		unsigned short* blkdata_zpos = malloc(blklen*2);
		unsigned char* blkdata_visfaces = malloc(blklen);
		unsigned char* blkdata_lighting = malloc(blklen);
		for(int k=0;k<blklen;k++) {
			blkdata_color[k] = buffer_read32(bytes,index);
			index += 4;
			blkdata_zpos[k] = buffer_read16(bytes,index);
			index += 2;
			blkdata_visfaces[k] = buffer_read8(bytes,index++); //0x00zZyYxX
			blkdata_lighting[k] = buffer_read8(bytes,index++); //compressed normal vector (also referred to as lighting)
		}
		index += 4*kv6->xsiz;
		kv6->voxels = malloc(kv6->xsiz*kv6->ysiz*kv6->zsiz*sizeof(struct kv6_voxel));
		kv6->voxel_count = 0;
		for(int x=0;x<kv6->xsiz;x++) {
			for(int y=0;y<kv6->ysiz;y++) {
				int size = buffer_read16(bytes,index);
				index += 2;
				for(int z=0;z<size;z++) {
					struct kv6_voxel* voxel = &kv6->voxels[kv6->voxel_count];
					voxel->x = x;
					voxel->y = y;
					voxel->z = (kv6->zsiz-1)-blkdata_zpos[kv6->voxel_count];
					voxel->color = (blkdata_color[kv6->voxel_count]&0xFFFFFF)|(blkdata_lighting[kv6->voxel_count]<<24);
					voxel->visfaces = blkdata_visfaces[kv6->voxel_count];
					kv6->voxel_count++;
				}
			}
		}
		kv6->voxels = realloc(kv6->voxels,kv6->voxel_count*sizeof(struct kv6_voxel));
		free(blkdata_color);
		free(blkdata_zpos);
		free(blkdata_visfaces);
		free(blkdata_lighting);
	}
}

char kv6_intersection(struct kv6_t* kv6, Ray* r) {
	AABB bb;

	for(int k=0;k<8;k++) {
		float v[4] = {(kv6->xsiz*((k&1)>0)-kv6->xpiv)*kv6->scale,(kv6->zsiz*((k&2)>0)-kv6->zpiv)*kv6->scale,(kv6->ysiz*((k&4)>0)-kv6->ypiv)*kv6->scale,1.0F};
		matrix_vector(v);
		bb.min_x = (k==0)?v[0]:min(bb.min_x,v[0]);
		bb.min_y = (k==0)?v[1]:min(bb.min_y,v[1]);
		bb.min_z = (k==0)?v[2]:min(bb.min_z,v[2]);

		bb.max_x = (k==0)?v[0]:max(bb.max_x,v[0]);
		bb.max_y = (k==0)?v[1]:max(bb.max_y,v[1]);
		bb.max_z = (k==0)?v[2]:max(bb.max_z,v[2]);
	}

	return aabb_intersection_ray(&bb,r);
}

void kv6_rebuild(struct kv6_t* kv6) {
	if(kv6->has_display_list) {
		glDeleteLists(kv6->display_list,3);
		if(kv6->colorize) {
			free(kv6->vertices_final);
			free(kv6->colors_final);
			free(kv6->normals_final);
		}
		kv6->has_display_list = 0;
	}
}

void kv6_render(struct kv6_t* kv6, unsigned char team) {
	//if(!network_logged_in)
		//return;

	if(!kv6->has_display_list) {
		int size = kv6->voxel_count*6;

		kv6->vertices_final = malloc(size*12*sizeof(float));
		kv6->colors_final = malloc(size*12*sizeof(unsigned char)*2);
		kv6->normals_final = malloc(size*12*sizeof(unsigned char));

		if(!kv6->colorize) {
			kv6->display_list = glGenLists(3);
			glxcheckErrors(__FILE__,__LINE__);
		}

		int v,c,n;
		for(int t=0;t<3;t++) {
			v = c = n = 0;

			/*for(int x=0;x<kv6->xsiz;x++) {
				for(int y=0;y<kv6->ysiz;y++) {
					for(int z=0;z<kv6->zsiz;z++) {*/
					for(int k=0;k<kv6->voxel_count;k++) {

						if(kv6->voxels[k].color!=0) {
							int x = kv6->voxels[k].x;
							int y = kv6->voxels[k].y;
							int z = kv6->voxels[k].z;
							int vis = kv6->voxels[k].visfaces; //zZyYxX
							int b = kv6->voxels[k].color & 0xFF;
							int g = (kv6->voxels[k].color>>8) & 0xFF;
							int r = (kv6->voxels[k].color>>16) & 0xFF;
							int a = (kv6->voxels[k].color>>24) & 0xFF;
							if(r==0 && g==0 && b==0) {
								switch(t) {
									case TEAM_1:
										r = gamestate.team_1.red;
										g = gamestate.team_1.green;
										b = gamestate.team_1.blue;
										break;
									case TEAM_2:
										r = gamestate.team_2.red;
										g = gamestate.team_2.green;
										b = gamestate.team_2.blue;
										break;
									default:
										r = g = b = 0;
								}
							}

							float p[3] = {(x-kv6->xpiv)*kv6->scale,(z-kv6->zpiv)*kv6->scale,(y-kv6->ypiv)*kv6->scale};

							int i = 0;
							float alpha[6];

							//negative y
							if(vis&16) {//z==kv6->zsiz-1 || !kv6->color[x+(y+(z+1)*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								alpha[i++] = 1.0F;
							}

							//positive y
							if(vis&32) {//z==0 || !kv6->color[x+(y+(z-1)*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								alpha[i++] = 0.6F;
							}

							//negative z
							if(vis&4) {//y==0 || !kv6->color[x+((y-1)+z*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								alpha[i++] = 0.95F;
							}

							//positive z
							if(vis&8) {//y==kv6->ysiz-1 || !kv6->color[x+((y+1)+z*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								alpha[i++] = 0.9F;
							}

							//negative x
							if(vis&1) {//x==0 || !kv6->color[(x-1)+(y+z*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0];
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								alpha[i++] = 0.85F;
							}

							//positive x
							if(vis&2) {//x==kv6->xsiz-1 || !kv6->color[(x+1)+(y+z*kv6->ysiz)*kv6->xsiz]) {
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2];
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1]+kv6->scale;
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								kv6->vertices_final[v++] = p[0]+kv6->scale;
								kv6->vertices_final[v++] = p[1];
								kv6->vertices_final[v++] = p[2]+kv6->scale;
								alpha[i++] = 0.8F;
							}

							for(int k=0;k<i*4;k++) {
								kv6->colors_final[c++] = r*alpha[k/4];//(r/255.0F*(a/255.0F*0.5F+0.5F))*255.0F;//*alpha[k/4];
								kv6->colors_final[c++] = g*alpha[k/4];//(g/255.0F*(a/255.0F*0.5F+0.5F))*255.0F;//*alpha[k/4];
								kv6->colors_final[c++] = b*alpha[k/4];//(b/255.0F*(a/255.0F*0.5F+0.5F))*255.0F;//*alpha[k/4];

								kv6->normals_final[n++] = kv6_normals[a][0]*128;
								kv6->normals_final[n++] = -kv6_normals[a][2]*128;
								kv6->normals_final[n++] = kv6_normals[a][1]*128;
							}
						}
					//}
				//}
			}

			if(!kv6->colorize) {
				glNewList(kv6->display_list+t,GL_COMPILE);
				glEnable(GL_NORMALIZE);
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_COLOR_ARRAY);
				glEnableClientState(GL_NORMAL_ARRAY);
				glVertexPointer(3,GL_FLOAT,0,kv6->vertices_final);
				glColorPointer(3,GL_UNSIGNED_BYTE,0,kv6->colors_final);
				glNormalPointer(GL_BYTE,0,kv6->normals_final);
				glDrawArrays(GL_QUADS,0,v/3);
				glDisableClientState(GL_NORMAL_ARRAY);
				glDisableClientState(GL_COLOR_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glDisable(GL_NORMALIZE);
				glEndList();
				glxcheckErrors(__FILE__,__LINE__);
			}
		}

		if(!kv6->colorize) {
			free(kv6->vertices_final);
			free(kv6->colors_final);
			free(kv6->normals_final);
		} else {
			memcpy(kv6->colors_final+v*sizeof(unsigned char),kv6->colors_final,v*sizeof(unsigned char));
			kv6->size = v/3;
		}

		kv6->has_display_list = 1;
	} else {
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);
		if(!kv6->colorize) {
			glCallList(kv6->display_list+(team%3));
		} else {
			for(int k=0;k<kv6->size*3;k+=3) {
				kv6->colors_final[k+0] = min(((float)kv6->colors_final[k+0+kv6->size*3*sizeof(unsigned char)])/0.4335F*kv6->red,255);
				kv6->colors_final[k+1] = min(((float)kv6->colors_final[k+1+kv6->size*3*sizeof(unsigned char)])/0.4335F*kv6->green,255);
				kv6->colors_final[k+2] = min(((float)kv6->colors_final[k+2+kv6->size*3*sizeof(unsigned char)])/0.4335F*kv6->blue,255);
			}
			glEnable(GL_NORMALIZE);
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);
			glEnableClientState(GL_NORMAL_ARRAY);
			glVertexPointer(3,GL_FLOAT,0,kv6->vertices_final);
			glColorPointer(3,GL_UNSIGNED_BYTE,0,kv6->colors_final);
			glNormalPointer(GL_BYTE,0,kv6->normals_final);
			glDrawArrays(GL_QUADS,0,kv6->size);
			glDisableClientState(GL_NORMAL_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisable(GL_NORMALIZE);
		}
		glDisable(GL_COLOR_MATERIAL);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
	}



	//TODO: render like in voxlap
	/*double m[16];
	glGetDoublev(GL_MODELVIEW_MATRIX,m);
	double p[16];
	glGetDoublev(GL_PROJECTION_MATRIX,p);

	double res[16];
	mul_matrix_matrix(res,p,m);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	float near_plane_h = (settings.window_height/(2.0F*tan(0.5F*camera_fov*3.141592F/180.0F)))*1.5F*scale;
	float w = near_plane_h/settings.window_width;
	float h = near_plane_h/settings.window_height;

	int k = 0;

	for(int x=0;x<kv6->xsiz;x++) {
		for(int y=0;y<kv6->ysiz;y++) {
			for(int z=0;z<kv6->zsiz;z++) {
				if(kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]!=0) {
					int b = kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz] & 0xFF;
					int g = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>8) & 0xFF;
					int r = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>16) & 0xFF;
					int a = (kv6->color[x+y*kv6->xsiz+z*kv6->xsiz*kv6->ysiz]>>24) & 0xFF;
					if(r==0 && g==0 && b==0) {
						switch(team) {
							case TEAM_1:
								r = gamestate.team_1.red;
								g = gamestate.team_1.green;
								b = gamestate.team_1.blue;
								break;
							case TEAM_2:
								r = gamestate.team_2.red;
								g = gamestate.team_2.green;
								b = gamestate.team_2.blue;
								break;
							default:
								r = g = b = 255;
						}
					}

					float n[4] = {kv6_normals[a][0],-kv6_normals[a][2],kv6_normals[a][1],0.0F};
					mul_matrix_vector(n,m,n);
					float len = sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
					n[0] /= len;
					n[1] /= len;
					n[2] /= len;

					float f = max(map_sun[0]*n[0]+map_sun[1]*n[1]+map_sun[2]*n[2],0.0F)*0.4F+0.6F;
					for(int i=(k/4*3);i<(k/4*3)+12;) {
						kv6->display_list_colors[i++] = r*f;
						kv6->display_list_colors[i++] = g*f;
						kv6->display_list_colors[i++] = b*f;
					}

					float v[4] = {(x-kv6->xpiv)*scale+tx,(z-kv6->zpiv)*scale+ty,(y-kv6->ypiv)*scale+tz,1.0F};
					mul_matrix_vector(v,res,v);

					kv6->display_list_vertices[k++]  = v[0]-w;
					kv6->display_list_vertices[k++]  = v[1]-h;
					kv6->display_list_vertices[k++]  = v[2];
					kv6->display_list_vertices[k++]  = v[3];

					kv6->display_list_vertices[k++]  = v[0]+w;
					kv6->display_list_vertices[k++]  = v[1]-h;
					kv6->display_list_vertices[k++]  = v[2];
					kv6->display_list_vertices[k++]  = v[3];

					kv6->display_list_vertices[k++]  = v[0]+w;
					kv6->display_list_vertices[k++]  = v[1]+h;
					kv6->display_list_vertices[k++] = v[2];
					kv6->display_list_vertices[k++] = v[3];

					kv6->display_list_vertices[k++] = v[0]-w;
					kv6->display_list_vertices[k++] = v[1]+h;
					kv6->display_list_vertices[k++] = v[2];
					kv6->display_list_vertices[k++] = v[3];
				}
			}
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(4,GL_FLOAT,0,kv6->display_list_vertices);
	glColorPointer(3,GL_UNSIGNED_BYTE,0,kv6->display_list_colors);
	glDrawArrays(GL_QUADS,0,k/4);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);*/
}
