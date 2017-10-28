#include "common.h"

float weapon_reload_start, weapon_last_shot;
unsigned char weapon_reload_inprogress = 0;

void weapon_update() {
    float t, delay = weapon_delay();
    int bullets = weapon_can_reload();
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            t = 2.5F;
            break;
        case WEAPON_SMG:
            t = 2.5F;
            break;
        case WEAPON_SHOTGUN:
            t = 0.5F*bullets;
            break;
    }

    if(weapon_reload_inprogress) {
        if(players[local_player_id].weapon==WEAPON_SHOTGUN) {
            if(glfwGetTime()-weapon_reload_start>=0.5F) {
                local_player_ammo++;
                local_player_ammo_reserved--;
                if(local_player_ammo<6) {
                    weapon_reload_start = glfwGetTime();
                } else {
                    weapon_reload_inprogress = 0;
                }
            }
        } else {
            if(glfwGetTime()-weapon_reload_start>=t) {
                local_player_ammo += bullets;
                local_player_ammo_reserved -= bullets;
                weapon_reload_inprogress = 0;
            }
        }
        players[local_player_id].item_disabled = glfwGetTime();
        players[local_player_id].items_show_start = glfwGetTime();
        players[local_player_id].items_show = 1;
    } else {
        if(glfwGetTime()-players[local_player_id].item_disabled>=0.5F) {
            if(players[local_player_id].input.buttons.lmb && players[local_player_id].held_item==TOOL_GUN && glfwGetTime()-weapon_last_shot>=delay && local_player_ammo>0) {
                weapon_shoot();
                local_player_ammo = max(local_player_ammo-1,0);
                weapon_last_shot = glfwGetTime();
            }
        }
    }
    //printf("%i %f %i %i\n",weapon_reload_inprogress,glfwGetTime()-weapon_reload_start,local_player_ammo,local_player_ammo_reserved);
}

float weapon_delay() {
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            return 0.5F;
        case WEAPON_SMG:
            return 0.1F;
        case WEAPON_SHOTGUN:
            return 1.0F;
    }
}

void weapon_set() {
    players[local_player_id].weapon = WEAPON_SMG;
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            local_player_ammo = 10;
            local_player_ammo_reserved = 50;
            break;
        case WEAPON_SMG:
            local_player_ammo = 30;
            local_player_ammo_reserved = 120;
            break;
        case WEAPON_SHOTGUN:
            local_player_ammo = 6;
            local_player_ammo_reserved = 48;
            break;
    }
    weapon_reload_inprogress = 0;
}

void weapon_reload() {
    if(local_player_ammo_reserved==0 || weapon_reload_inprogress || !weapon_can_reload()) {
        return;
    }
    weapon_reload_start = glfwGetTime();
    weapon_reload_inprogress = 1;

    struct Sound_wav* reload;
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            reload = &sound_rifle_reload;
            break;
        case WEAPON_SMG:
            reload = &sound_smg_reload;
            break;
        case WEAPON_SHOTGUN:
            reload = &sound_shotgun_reload;
            break;
    }

    sound_create(NULL,SOUND_LOCAL,reload,players[local_player_id].pos.x,players[local_player_id].pos.y,players[local_player_id].pos.z);

    struct PacketWeaponReload reloadp;
    reloadp.player_id = local_player_id;
    reloadp.ammo = local_player_ammo;
    reloadp.reserved = local_player_ammo_reserved;
    network_send(PACKET_WEAPONRELOAD_ID,&reloadp,sizeof(reloadp));
}

void weapon_reload_abort() {
    if(weapon_reload_inprogress && players[local_player_id].weapon==WEAPON_SHOTGUN) {
        weapon_reload_inprogress = 0;
    }
}

int weapon_reloading() {
    return weapon_reload_inprogress;
}

int weapon_can_reload() {
    int mag_size;
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            mag_size = 10;
            break;
        case WEAPON_SMG:
            mag_size = 30;
            break;
        case WEAPON_SHOTGUN:
            mag_size = 6;
            break;
    }
    return max(min(min(local_player_ammo_reserved,mag_size),mag_size-local_player_ammo),0);
}

void weapon_shoot() {
    //see this for reference:
    //https://pastebin.com/raw/TMjKSTXG
    //http://paste.quacknet.org/view/a3ea2743

    double horiz_recoil, vert_recoil;
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            horiz_recoil = 0.0001;
            vert_recoil = 0.050000001;
            break;
        case WEAPON_SMG:
            horiz_recoil = 0.00005;
            vert_recoil = 0.0125;
            break;
        case WEAPON_SHOTGUN:
            horiz_recoil = 0.0002;
            vert_recoil = 0.1;
            break;
    }

    long triangle_wave = (long)(glfwGetTime()*1000)&511;
    horiz_recoil *= ((double)triangle_wave-255.5);

    if(((long)(glfwGetTime()*1000)&1023)<512) {
        horiz_recoil *= -1.0;
    }

    if((players[local_player_id].input.keys.up
        || players[local_player_id].input.keys.down
        || players[local_player_id].input.keys.left
        || players[local_player_id].input.keys.right)
        && !players[local_player_id].input.buttons.rmb) {
        vert_recoil *= 2.0;
        horiz_recoil *= 2.0;
    }
    if(players[local_player_id].physics.airborne) {
        vert_recoil *= 2.0;
        horiz_recoil *= 2.0;
    } else {
        if(players[local_player_id].input.keys.crouch) {
            vert_recoil *= 0.5;
            horiz_recoil *= 0.5;
        }
    }

    /*horiz_recoil *= sqrt(1.0F-players[local_player_id].orientation.y
                            *players[local_player_id].orientation.y
                            *players[local_player_id].orientation.y
                            *players[local_player_id].orientation.y);*/

    camera_rot_x += horiz_recoil;
    camera_rot_y -= vert_recoil;

    struct Sound_wav* shoot;
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            shoot = &sound_rifle_shoot;
            break;
        case WEAPON_SMG:
            shoot = &sound_smg_shoot;
            break;
        case WEAPON_SHOTGUN:
            shoot = &sound_shotgun_shoot;
            break;
    }

    sound_create(NULL,SOUND_LOCAL,shoot,players[local_player_id].pos.x,players[local_player_id].pos.y,players[local_player_id].pos.z);

    camera_overflow_adjust();
}
