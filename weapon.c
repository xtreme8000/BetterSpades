void weapon_update() {
    float t, delay;
    int bullets = weapon_can_reload();
    switch(players[local_player_id].weapon) {
        case WEAPON_RIFLE:
            t = 2.5F;
            delay = 0.5F;
            break;
        case WEAPON_SMG:
            t = 2.5F;
            delay = 0.1F;
            break;
        case WEAPON_SHOTGUN:
            t = 0.5F*bullets;
            delay = 1.0F;
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
            if(players[local_player_id].input.buttons.lmb && players[local_player_id].held_item==TOOL_GUN && glfwGetTime()-weapon_last_shot>=delay) {
                local_player_ammo = max(local_player_ammo-1,0);
                weapon_last_shot = glfwGetTime();
            }
        }
    }
    //printf("%i %f %i %i\n",weapon_reload_inprogress,glfwGetTime()-weapon_reload_start,local_player_ammo,local_player_ammo_reserved);
}

void weapon_set() {
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
