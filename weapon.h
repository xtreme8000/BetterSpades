void weapon_update();
void weapon_set();
void weapon_reload();
int weapon_reloading();
int weapon_can_reload();
void weapon_reload_abort();

float weapon_reload_start, weapon_last_shot;
unsigned char weapon_reload_inprogress = 0;
