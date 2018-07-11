/*
	Copyright (c) 2017-2018 ByteBit

	This file is part of BetterSpades.

    BetterSpades is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BetterSpades is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BetterSpades.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common.h"

struct RENDER_OPTIONS settings;
struct list config_keys;

static int config_read_key(void* user, const char* section, const char* name, const char* value) {
    if(!strcmp(section,"client")) {
        if(!strcmp(name,"name")) {
            strcpy(settings.name,value);
        }
        if(!strcmp(name,"xres")) {
            settings.window_width = atoi(value);
        }
        if(!strcmp(name,"yres")) {
            settings.window_height = atoi(value);
        }
        if(!strcmp(name,"windowed")) {
            settings.fullscreen = !atoi(value);
        }
        if(!strcmp(name,"multisamples")) {
            settings.multisamples = atoi(value);
        }
        if(!strcmp(name,"greedy_meshing")) {
            settings.greedy_meshing = atoi(value);
        }
        if(!strcmp(name,"vsync")) {
            settings.vsync = atoi(value);
        }
        if(!strcmp(name,"mouse_sensitivity")) {
            settings.mouse_sensitivity = atof(value)/5.0F*MOUSE_SENSITIVITY;
        }
        if(!strcmp(name,"show_news")) {
            settings.show_news = atoi(value);
        }
        if(!strcmp(name,"vol")) {
            sound_global_volume = max(min(atoi(value),10),0);
            sound_volume(sound_global_volume/10.0F);
        }
        if(!strcmp(name,"show_fps")) {
            settings.show_fps = atoi(value);
        }
    }
    if(!strcmp(section,"controls")) {
        for(int k=0;k<list_size(&config_keys);k++) {
            struct config_key_pair* key = list_get(&config_keys,k);
            if(!strcmp(name,key->name)) {
                //printf("found override for %s, from %i to %i\n",key->name,key->def,atoi(value));
                key->def = strtol(value,NULL,0);
                break;
            }
        }
    }
    return 1;
}

void config_register_key(int internal, int def, const char* name, int toggle) {
    struct config_key_pair key;
    key.internal = internal;
    key.def = def;
    key.toggle = toggle;
    if(name)
        strcpy(key.name,name);
    else
        *key.name = 0;
    list_add(&config_keys,&key);
}

int config_key_translate(int key, int dir) {
    for(int k=0;k<list_size(&config_keys);k++) {
        struct config_key_pair* a = list_get(&config_keys,k);
        if(dir && a->internal==key)
            return a->def;
        if(!dir && a->def==key)
            return a->internal;
    }
    return -1;
}

struct config_key_pair* config_key(int key) {
    for(int k=0;k<list_size(&config_keys);k++) {
        struct config_key_pair* a = list_get(&config_keys,k);
        if(a->internal==key)
            return a;
    }
    return NULL;
}

void config_key_reset_togglestates() {
    for(int k=0;k<list_size(&config_keys);k++) {
        struct config_key_pair* a = list_get(&config_keys,k);
        if(a->toggle)
            window_pressed_keys[a->internal] = 0;
    }
}

void config_reload() {
    if(!list_created(&config_keys))
        list_create(&config_keys,sizeof(struct config_key_pair));
    else
        list_clear(&config_keys);

	#ifdef USE_SDL
    config_register_key(WINDOW_KEY_UP,SDLK_w,"move_forward",0);
    config_register_key(WINDOW_KEY_DOWN,SDLK_s,"move_backward",0);
    config_register_key(WINDOW_KEY_LEFT,SDLK_a,"move_left",0);
    config_register_key(WINDOW_KEY_RIGHT,SDLK_d,"move_right",0);
    config_register_key(WINDOW_KEY_SPACE,SDLK_SPACE,"jump",0);
    config_register_key(WINDOW_KEY_SPRINT,SDLK_LSHIFT,"sprint",0);
    config_register_key(WINDOW_KEY_CURSOR_UP,SDLK_UP,"cube_color_up",0);
    config_register_key(WINDOW_KEY_CURSOR_DOWN,SDLK_DOWN,"cube_color_down",0);
    config_register_key(WINDOW_KEY_CURSOR_LEFT,SDLK_LEFT,"cube_color_left",0);
    config_register_key(WINDOW_KEY_CURSOR_RIGHT,SDLK_RIGHT,"cube_color_right",0);
    config_register_key(WINDOW_KEY_BACKSPACE,SDLK_BACKSPACE,NULL,0);
    config_register_key(WINDOW_KEY_TOOL1,SDLK_1,NULL,0);
    config_register_key(WINDOW_KEY_TOOL2,SDLK_2,NULL,0);
    config_register_key(WINDOW_KEY_TOOL3,SDLK_3,NULL,0);
    config_register_key(WINDOW_KEY_TOOL4,SDLK_4,NULL,0);
    config_register_key(WINDOW_KEY_TAB,SDLK_TAB,"view_score",0);
    config_register_key(WINDOW_KEY_ESCAPE,SDLK_ESCAPE,"quit_game",0);
    config_register_key(WINDOW_KEY_MAP,SDLK_m,"view_map",1);
    config_register_key(WINDOW_KEY_CROUCH,SDLK_LCTRL,"crouch",0);
    config_register_key(WINDOW_KEY_SNEAK,SDLK_v,"sneak",0);
    config_register_key(WINDOW_KEY_ENTER,SDLK_RETURN,NULL,0);
    config_register_key(WINDOW_KEY_F1,SDLK_F1,NULL,0);
    config_register_key(WINDOW_KEY_F2,SDLK_F2,NULL,0);
    config_register_key(WINDOW_KEY_F3,SDLK_F3,NULL,0);
    config_register_key(WINDOW_KEY_F4,SDLK_F4,NULL,0);
    config_register_key(WINDOW_KEY_YES,SDLK_y,NULL,0);
    config_register_key(WINDOW_KEY_YES,SDLK_z,NULL,0);
    config_register_key(WINDOW_KEY_NO,SDLK_n,NULL,0);
    config_register_key(WINDOW_KEY_VOLUME_UP,SDLK_KP_PLUS,"volume_up",0);
    config_register_key(WINDOW_KEY_VOLUME_DOWN,SDLK_KP_MINUS,"volume_down",0);
    config_register_key(WINDOW_KEY_V,SDLK_v,NULL,0);
    config_register_key(WINDOW_KEY_RELOAD,SDLK_r,"reload",0);
    config_register_key(WINDOW_KEY_CHAT,SDLK_t,"chat_global",0);
    config_register_key(WINDOW_KEY_FULLSCREEN,SDLK_F11,"fullscreen",0);
    config_register_key(WINDOW_KEY_SCREENSHOT,SDLK_F5,"screenshot",0);
    config_register_key(WINDOW_KEY_CHANGETEAM,SDLK_COMMA,"change_team",0);
    config_register_key(WINDOW_KEY_CHANGEWEAPON,SDLK_PERIOD,"change_weapon",0);
    config_register_key(WINDOW_KEY_PICKCOLOR,SDLK_e,"cube_color_sample",0);
    config_register_key(WINDOW_KEY_COMMAND,SDLK_SLASH,"chat_command",0);
    config_register_key(WINDOW_KEY_HIDEHUD,SDLK_F6,"hide_hud",1);
	#endif
	
	#ifdef USE_GLFW
	config_register_key(WINDOW_KEY_UP,GLFW_KEY_W,"move_forward",0);
    config_register_key(WINDOW_KEY_DOWN,GLFW_KEY_S,"move_backward",0);
    config_register_key(WINDOW_KEY_LEFT,GLFW_KEY_A,"move_left",0);
    config_register_key(WINDOW_KEY_RIGHT,GLFW_KEY_D,"move_right",0);
    config_register_key(WINDOW_KEY_SPACE,GLFW_KEY_SPACE,"jump",0);
    config_register_key(WINDOW_KEY_SPRINT,GLFW_KEY_LEFT_SHIFT,"sprint",0);
    config_register_key(WINDOW_KEY_CURSOR_UP,GLFW_KEY_UP,"cube_color_up",0);
    config_register_key(WINDOW_KEY_CURSOR_DOWN,GLFW_KEY_DOWN,"cube_color_down",0);
    config_register_key(WINDOW_KEY_CURSOR_LEFT,GLFW_KEY_LEFT,"cube_color_left",0);
    config_register_key(WINDOW_KEY_CURSOR_RIGHT,GLFW_KEY_RIGHT,"cube_color_right",0);
    config_register_key(WINDOW_KEY_BACKSPACE,GLFW_KEY_BACKSPACE,NULL,0);
    config_register_key(WINDOW_KEY_TOOL1,GLFW_KEY_1,NULL,0);
    config_register_key(WINDOW_KEY_TOOL2,GLFW_KEY_2,NULL,0);
    config_register_key(WINDOW_KEY_TOOL3,GLFW_KEY_3,NULL,0);
    config_register_key(WINDOW_KEY_TOOL4,GLFW_KEY_4,NULL,0);
    config_register_key(WINDOW_KEY_TAB,GLFW_KEY_TAB,"view_score",0);
    config_register_key(WINDOW_KEY_ESCAPE,GLFW_KEY_ESCAPE,"quit_game",0);
    config_register_key(WINDOW_KEY_MAP,GLFW_KEY_M,"view_map",1);
    config_register_key(WINDOW_KEY_CROUCH,GLFW_KEY_LEFT_CONTROL,"crouch",0);
    config_register_key(WINDOW_KEY_SNEAK,GLFW_KEY_V,"sneak",0);
    config_register_key(WINDOW_KEY_ENTER,GLFW_KEY_ENTER,NULL,0);
    config_register_key(WINDOW_KEY_F1,GLFW_KEY_F1,NULL,0);
    config_register_key(WINDOW_KEY_F2,GLFW_KEY_F2,NULL,0);
    config_register_key(WINDOW_KEY_F3,GLFW_KEY_F3,NULL,0);
    config_register_key(WINDOW_KEY_F4,GLFW_KEY_F4,NULL,0);
    config_register_key(WINDOW_KEY_YES,GLFW_KEY_Y,NULL,0);
    config_register_key(WINDOW_KEY_YES,GLFW_KEY_Z,NULL,0);
    config_register_key(WINDOW_KEY_NO,GLFW_KEY_N,NULL,0);
    config_register_key(WINDOW_KEY_VOLUME_UP,GLFW_KEY_KP_ADD,"volume_up",0);
    config_register_key(WINDOW_KEY_VOLUME_DOWN,GLFW_KEY_KP_SUBTRACT,"volume_down",0);
    config_register_key(WINDOW_KEY_V,GLFW_KEY_V,NULL,0);
    config_register_key(WINDOW_KEY_RELOAD,GLFW_KEY_R,"reload",0);
    config_register_key(WINDOW_KEY_CHAT,GLFW_KEY_T,"chat_global",0);
    config_register_key(WINDOW_KEY_FULLSCREEN,GLFW_KEY_F11,"fullscreen",0);
    config_register_key(WINDOW_KEY_SCREENSHOT,GLFW_KEY_F5,"screenshot",0);
    config_register_key(WINDOW_KEY_CHANGETEAM,GLFW_KEY_COMMA,"change_team",0);
    config_register_key(WINDOW_KEY_CHANGEWEAPON,GLFW_KEY_PERIOD,"change_weapon",0);
    config_register_key(WINDOW_KEY_PICKCOLOR,GLFW_KEY_E,"cube_color_sample",0);
    config_register_key(WINDOW_KEY_COMMAND,GLFW_KEY_SLASH,"chat_command",0);
	config_register_key(WINDOW_KEY_HIDEHUD,GLFW_KEY_F6,"hide_hud",1);
	#endif

    ini_parse("config.ini",config_read_key,NULL);
}
