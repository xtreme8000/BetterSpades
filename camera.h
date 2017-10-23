#define CAMERAMODE_SELECTION    0
#define CAMERAMODE_FPS          1
#define CAMERAMODE_SPECTATOR    2
#define CAMERAMODE_BODYVIEW     3

#define CAMERA_DEFAULT_FOV      70.0F

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

void camera_ExtractFrustum(void);
unsigned char camera_PointInFrustum(float x, float y, float z);
int camera_CubeInFrustum(float x, float y, float z, float size, float size_y);
int* camera_terrain_pick(unsigned char mode);
void camera_overflow_adjust(void);
