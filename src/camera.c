#include "common.h"

unsigned char camera_mode = CAMERAMODE_SPECTATOR;

float frustum[6][4];
float camera_rot_x = 2.04F, camera_rot_y = 1.79F;
float camera_x = 256.0F, camera_y = 60.0F, camera_z = 256.0F;
float camera_vx, camera_vy, camera_vz;
float camera_fov = CAMERA_DEFAULT_FOV;
float camera_size = 0.8F;
float camera_height = 0.8F;
float camera_eye_height = 0.0F;
float camera_movement_x = 0.0F, camera_movement_y = 0.0F, camera_movement_z = 0.0F;
float camera_speed = 32.0F;
long camera_last_key = 0;

void camera_overflow_adjust() {
	if(camera_rot_y<EPSILON) {
		camera_rot_y = EPSILON;
	}
	if(camera_rot_y>PI) {
		camera_rot_y = PI;
	}

	if(camera_rot_x>DOUBLEPI) {
		camera_rot_x -= DOUBLEPI;
	}
	if(camera_rot_x<0.0F) {
		camera_rot_x += DOUBLEPI;
	}
}

void camera_apply(float dt) {
	switch(camera_mode) {
		case CAMERAMODE_FPS:
			cameracontroller_fps(dt);
			break;
		case CAMERAMODE_BODYVIEW:
			cameracontroller_bodyview(dt);
			break;
		case CAMERAMODE_SPECTATOR:
			cameracontroller_spectator(dt);
			break;
		case CAMERAMODE_SELECTION:
			cameracontroller_selection(dt);
			break;
	}
}

int* camera_terrain_pick(unsigned char mode) {
	return camera_terrain_pickEx(mode,camera_x,camera_y,camera_z,sin(camera_rot_x)*sin(camera_rot_y),cos(camera_rot_y),cos(camera_rot_x)*sin(camera_rot_y));
}

int* camera_terrain_pickEx(unsigned char mode, float x, float y, float z, float ray_x, float ray_y, float ray_z) {
	//naive approach until I find something better without glitches
	ray_x *= 0.01F;
	ray_y *= 0.01F;
	ray_z *= 0.01F;

	if(mode==0) {
		unsigned long long now,next;
		for(int k=0;k<2000;k++) {
			now = map_get(x,y,z);
			next = map_get(x+ray_x,y+ray_y,z+ray_z);
			if(next!=0xFFFFFFFF && now==0xFFFFFFFF) {
				if(floor(y+ray_y)<1 || floor(y)<1) {
					return (int*)0;
				}
				static int ret[3];
				ret[0] = floor(x);
				ret[1] = floor(y);
				ret[2] = floor(z);
				return ret;
			}
			x += ray_x;
			y += ray_y;
			z += ray_z;
		}
	} else {
		if(mode==1) {
			for(int k=0;k<2000;k++) {
				if(floor(y)>0 && map_get(x,y,z)!=0xFFFFFFFF) {
					static int ret[3];
					ret[0] = floor(x);
					ret[1] = floor(y);
					ret[2] = floor(z);
					return ret;
				}
				x += ray_x;
				y += ray_y;
				z += ray_z;
			}
		} else {
			for(int k=0;k<2000;k++) {
				if(map_get(x,y,z)!=0xFFFFFFFF) {
					static int ret[3];
					ret[0] = floor(x);
					ret[1] = floor(y);
					ret[2] = floor(z);
					return ret;
				}
				x += ray_x;
				y += ray_y;
				z += ray_z;
			}
		}
	}
	return (int*)0;
}

void camera_ExtractFrustum() {
   float   clip[16];
   float   t;

   matrix_push();
   matrix_load(matrix_model);
   matrix_multiply(matrix_view);
   matrix_multiply(matrix_projection);
   memcpy(clip,matrix_current,16*sizeof(float));
   matrix_pop();

   /* Extract the numbers for the RIGHT plane */
   frustum[0][0] = clip[ 3] - clip[ 0];
   frustum[0][1] = clip[ 7] - clip[ 4];
   frustum[0][2] = clip[11] - clip[ 8];
   frustum[0][3] = clip[15] - clip[12];

   /* Normalize the result */
   t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
   frustum[0][0] /= t;
   frustum[0][1] /= t;
   frustum[0][2] /= t;
   frustum[0][3] /= t;

   /* Extract the numbers for the LEFT plane */
   frustum[1][0] = clip[ 3] + clip[ 0];
   frustum[1][1] = clip[ 7] + clip[ 4];
   frustum[1][2] = clip[11] + clip[ 8];
   frustum[1][3] = clip[15] + clip[12];

   /* Normalize the result */
   t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
   frustum[1][0] /= t;
   frustum[1][1] /= t;
   frustum[1][2] /= t;
   frustum[1][3] /= t;

   /* Extract the BOTTOM plane */
   frustum[2][0] = clip[ 3] + clip[ 1];
   frustum[2][1] = clip[ 7] + clip[ 5];
   frustum[2][2] = clip[11] + clip[ 9];
   frustum[2][3] = clip[15] + clip[13];

   /* Normalize the result */
   t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
   frustum[2][0] /= t;
   frustum[2][1] /= t;
   frustum[2][2] /= t;
   frustum[2][3] /= t;

   /* Extract the TOP plane */
   frustum[3][0] = clip[ 3] - clip[ 1];
   frustum[3][1] = clip[ 7] - clip[ 5];
   frustum[3][2] = clip[11] - clip[ 9];
   frustum[3][3] = clip[15] - clip[13];

   /* Normalize the result */
   t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
   frustum[3][0] /= t;
   frustum[3][1] /= t;
   frustum[3][2] /= t;
   frustum[3][3] /= t;

   /* Extract the FAR plane */
   frustum[4][0] = clip[ 3] - clip[ 2];
   frustum[4][1] = clip[ 7] - clip[ 6];
   frustum[4][2] = clip[11] - clip[10];
   frustum[4][3] = clip[15] - clip[14];

   /* Normalize the result */
   t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
   frustum[4][0] /= t;
   frustum[4][1] /= t;
   frustum[4][2] /= t;
   frustum[4][3] /= t;

   /* Extract the NEAR plane */
   frustum[5][0] = clip[ 3] + clip[ 2];
   frustum[5][1] = clip[ 7] + clip[ 6];
   frustum[5][2] = clip[11] + clip[10];
   frustum[5][3] = clip[15] + clip[14];

   /* Normalize the result */
   t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
   frustum[5][0] /= t;
   frustum[5][1] /= t;
   frustum[5][2] /= t;
   frustum[5][3] /= t;
}

unsigned char camera_PointInFrustum(float x, float y, float z) {
   int p;

   for( p = 0; p < 6; p++ )
      if( frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= 0 )
         return 0;
   return 1;
}

int camera_CubeInFrustum( float x, float y, float z, float size, float size_y) {
   int p;
   int c;
   int c2 = 0;

   for( p = 0; p < 6; p++ )
   {
      c = 0;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y - size) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z - size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x - size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         c++;
      if( frustum[p][0] * (x + size) + frustum[p][1] * (y + size_y) + frustum[p][2] * (z + size) + frustum[p][3] > 0 )
         c++;
      if( c == 0 )
         return 0;
      if( c == 8 )
         c2++;
   }
   return (c2 == 6) ? 2 : 1;
}
