float frustum[6][4];
float camera_rot_x = 2.04F, camera_rot_y = 1.79F;
float camera_x = 221.36F, camera_y = 60.0F, camera_z = 332.15F;
float camera_fov = 70.0F;

void camera_ExtractFrustum();
unsigned char camera_PointInFrustum(float x, float y, float z);
int camera_CubeInFrustum(float x, float y, float z, float size, float size_y);