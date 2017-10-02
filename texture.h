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

void texture_init();
int texture_create(struct texture* t, char* filename);
int texture_create_buffer(struct texture* t, int width, int height, unsigned char* buff);
void texture_draw(struct texture* t, float x, float y, float w, float h);
