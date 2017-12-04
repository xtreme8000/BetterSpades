#define CAMERAMODE_SELECTION    0
#define CAMERAMODE_FPS          1
#define CAMERAMODE_SPECTATOR    2
#define CAMERAMODE_BODYVIEW     3

#define CAMERA_DEFAULT_FOV      70.0F

extern unsigned char camera_mode;

extern float frustum[6][4];
extern float camera_rot_x, camera_rot_y;
extern float camera_x, camera_y, camera_z;
extern float camera_vx, camera_vy, camera_vz;
extern float camera_fov;
extern float camera_size;
extern float camera_height;
extern float camera_eye_height;
extern float camera_movement_x, camera_movement_y, camera_movement_z;
extern float camera_speed;
extern long camera_last_key;

struct Camera_HitType {
	char type;
	float x,y,z,distance;
	unsigned char player_id, player_section;
};
#define CAMERA_HITTYPE_NONE		0
#define CAMERA_HITTYPE_BLOCK	1
#define CAMERA_HITTYPE_PLAYER	2

void camera_hit_fromplayer(struct Camera_HitType* hit, int player_id, float range);
void camera_hit(struct Camera_HitType* hit, int exclude_player, float x, float y, float z, float ray_x, float ray_y, float ray_z, float range);

void camera_ExtractFrustum(void);
unsigned char camera_PointInFrustum(float x, float y, float z);
int camera_CubeInFrustum(float x, float y, float z, float size, float size_y);
int* camera_terrain_pick(unsigned char mode);
int* camera_terrain_pickEx(unsigned char mode, float x, float y, float z, float ray_x, float ray_y, float ray_z);
void camera_overflow_adjust(void);
void camera_apply(float dt);
