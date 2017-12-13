void weapon_update(void);
void weapon_set(void);
void weapon_reload(void);
int weapon_reloading(void);
int weapon_can_reload(void);
void weapon_reload_abort(void);
void weapon_shoot(void);
int weapon_block_damage(int gun);
float weapon_delay(int gun);
struct Sound_wav* weapon_sound(int gun);
struct Sound_wav* weapon_sound_reload(int gun);
void weapon_spread(int gun, char scoped, float* out);

extern float weapon_reload_start, weapon_last_shot;
extern unsigned char weapon_reload_inprogress;
