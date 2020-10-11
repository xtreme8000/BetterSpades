/*
	Copyright (c) 2017-2020 ByteBit

	This file is part of BetterSpades.

	BetterSpades is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	BetterSpades is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CAMERA_H
#define CAMERA_H

enum camera_mode {
	CAMERAMODE_SELECTION,
	CAMERAMODE_FPS,
	CAMERAMODE_SPECTATOR,
	CAMERAMODE_BODYVIEW,
	CAMERAMODE_DEATH,
};

#define CAMERA_DEFAULT_FOV 70.0F
#define CAMERA_MAX_FOV 100.0F

extern enum camera_mode camera_mode;

extern float frustum[6][4];
extern float camera_rot_x, camera_rot_y;
extern float camera_x, camera_y, camera_z;
extern float camera_vx, camera_vy, camera_vz;
extern float camera_size;
extern float camera_height;
extern float camera_eye_height;
extern float camera_movement_x, camera_movement_y, camera_movement_z;
extern float camera_speed;

struct Camera_HitType {
	char type;
	float x, y, z, distance;
	int xb, yb, zb;
	unsigned char player_id, player_section;
};

#define CAMERA_HITTYPE_NONE 0
#define CAMERA_HITTYPE_BLOCK 1
#define CAMERA_HITTYPE_PLAYER 2

void camera_hit_fromplayer(struct Camera_HitType* hit, int player_id, float range);
void camera_hit(struct Camera_HitType* hit, int exclude_player, float x, float y, float z, float ray_x, float ray_y,
				float ray_z, float range);
void camera_hit_mask(struct Camera_HitType* hit, int exclude_player, float x, float y, float z, float ray_x,
					 float ray_y, float ray_z, float range);

float camera_fov_scaled();
void camera_ExtractFrustum(void);
unsigned char camera_PointInFrustum(float x, float y, float z);
int camera_CubeInFrustum(float x, float y, float z, float size, float size_y);
int* camera_terrain_pick(unsigned char mode);
int* camera_terrain_pickEx(unsigned char mode, float x, float y, float z, float ray_x, float ray_y, float ray_z);
void camera_overflow_adjust(void);
void camera_apply(void);
void camera_update(float dt);

#endif
