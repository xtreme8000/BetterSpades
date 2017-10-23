struct texture {
    int width,height;
    int texture_id;
    unsigned char* pixels;
};

struct texture texture_health;
struct texture texture_block;
struct texture texture_grenade;
struct texture texture_ammo_semi;
struct texture texture_ammo_smg;
struct texture texture_ammo_shotgun;

struct texture texture_color_selection;

struct texture texture_zoom_semi;
struct texture texture_zoom_smg;
struct texture texture_zoom_shotgun;

struct texture texture_white;

void texture_init(void);
int texture_create(struct texture* t, char* filename);
int texture_create_buffer(struct texture* t, int width, int height, unsigned char* buff);
void texture_draw(struct texture* t, float x, float y, float w, float h);
void texture_draw_empty(float x, float y, float w, float h);
void texture_resize_pow2(struct texture* t);
unsigned int texture_block_color(int x, int y);
