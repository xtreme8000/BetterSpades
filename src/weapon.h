void weapon_update(void);
void weapon_set(void);
void weapon_reload(void);
int weapon_reloading(void);
int weapon_can_reload(void);
void weapon_reload_abort(void);
void weapon_shoot(void);
float weapon_delay(void);

extern float weapon_reload_start, weapon_last_shot;
extern unsigned char weapon_reload_inprogress;
