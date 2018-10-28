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

struct RENDER_OPTIONS settings, settings_tmp;
struct list config_keys;
struct list config_settings;

struct list config_file;

static void config_sets(const char* name, const char* value) {
	for(int k=0;k<list_size(&config_file);k++) {
		struct config_file_entry* e = list_get(&config_file,k);
		if(strcmp(e->name,name)==0) {
			strncpy(e->value,value,sizeof(e->value)-1);
			break;
		}
	}
}

static void config_seti(const char* name, int value) {
	char tmp[32];
	sprintf(tmp,"%i",value);
	config_sets(name,tmp);
}

static void config_setf(const char* name, float value) {
	char tmp[32];
	sprintf(tmp,"%0.6f",value);
	config_sets(name,tmp);
}

void config_save() {
	kv6_rebuild_complete();

	config_sets("name",settings.name);
	config_seti("xres",settings.window_width);
	config_seti("yres",settings.window_height);
	config_seti("windowed",!settings.fullscreen);
	config_seti("multisamples",settings.multisamples);
	config_seti("greedy_meshing",settings.greedy_meshing);
	config_seti("vsync",settings.vsync);
	config_setf("mouse_sensitivity",settings.mouse_sensitivity);
	config_seti("show_news",settings.show_news);
	config_seti("vol",settings.volume);
	config_seti("show_fps",settings.show_fps);
	config_seti("voxlap_models",settings.voxlap_models);

	for(int k=0;k<list_size(&config_keys);k++) {
		struct config_key_pair* e = list_get(&config_keys,k);
		config_seti(e->name,e->def);
	}


	FILE* f = fopen("config.ini","w");
	char last_section[32] = {0};
	for(int k=0;k<list_size(&config_file);k++) {
		struct config_file_entry* e = list_get(&config_file,k);
		if(strcmp(e->section,last_section)!=0) {
			fprintf(f,"\r\n[%s]\r\n",e->section);
			strcpy(last_section,e->section);
		}
		fprintf(f,"%s",e->name);
		for(int l=0;l<31-strlen(e->name);l++)
			fprintf(f," ");
		fprintf(f,"= %s\r\n",e->value);
	}
	fclose(f);
}

static int config_read_key(void* user, const char* section, const char* name, const char* value) {
	struct config_file_entry e;
	strncpy(e.section,section,sizeof(e.section)-1);
	strncpy(e.name,name,sizeof(e.name)-1);
	strncpy(e.value,value,sizeof(e.value)-1);
	list_add(&config_file,&e);

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
            settings.mouse_sensitivity = atof(value);
        }
        if(!strcmp(name,"show_news")) {
            settings.show_news = atoi(value);
        }
        if(!strcmp(name,"vol")) {
            settings.volume = max(min(atoi(value),10),0);
            sound_volume(settings.volume/10.0F);
        }
        if(!strcmp(name,"show_fps")) {
            settings.show_fps = atoi(value);
        }
		if(!strcmp(name,"voxlap_models")) {
			settings.voxlap_models = atoi(value);
		}
    }
    if(!strcmp(section,"controls")) {
        for(int k=0;k<list_size(&config_keys);k++) {
            struct config_key_pair* key = list_get(&config_keys,k);
            if(!strcmp(name,key->name)) {
                log_debug("found override for %s, from %i to %i",key->name,key->def,atoi(value));
                key->def = strtol(value,NULL,0);
                break;
            }
        }
    }
    return 1;
}

void config_register_key(int internal, int def, const char* name, int toggle, const char* display) {
	struct config_key_pair key;
	key.internal = internal;
	key.def = def;
	key.toggle = toggle;
	if(display)
		strncpy(key.display,display,sizeof(key.display)-1);
	else
		*key.display = 0;

	if(name)
		strncpy(key.name,name,sizeof(key.name)-1);
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
	if(!list_created(&config_file))
		list_create(&config_file,sizeof(struct config_file_entry));
	else
		list_clear(&config_file);


	if(!list_created(&config_keys))
		list_create(&config_keys,sizeof(struct config_key_pair));
	else
		list_clear(&config_keys);

	#ifdef USE_SDL
	config_register_key(WINDOW_KEY_UP,SDLK_w,"move_forward",0,"Forward");
	config_register_key(WINDOW_KEY_DOWN,SDLK_s,"move_backward",0,"Backward");
	config_register_key(WINDOW_KEY_LEFT,SDLK_a,"move_left",0,"Left");
	config_register_key(WINDOW_KEY_RIGHT,SDLK_d,"move_right",0,"Right");
	config_register_key(WINDOW_KEY_SPACE,SDLK_SPACE,"jump",0,"Jump");
	config_register_key(WINDOW_KEY_SPRINT,SDLK_LSHIFT,"sprint",0,"Sprint");
	config_register_key(WINDOW_KEY_CURSOR_UP,SDLK_UP,"cube_color_up",0,"Color up");
	config_register_key(WINDOW_KEY_CURSOR_DOWN,SDLK_DOWN,"cube_color_down",0,"Color down");
	config_register_key(WINDOW_KEY_CURSOR_LEFT,SDLK_LEFT,"cube_color_left",0,"Color left");
	config_register_key(WINDOW_KEY_CURSOR_RIGHT,SDLK_RIGHT,"cube_color_right",0,"Color right");
	config_register_key(WINDOW_KEY_BACKSPACE,SDLK_BACKSPACE,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL1,SDLK_1,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL2,SDLK_2,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL3,SDLK_3,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL4,SDLK_4,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TAB,SDLK_TAB,"view_score",0,"Score");
	config_register_key(WINDOW_KEY_ESCAPE,SDLK_ESCAPE,"quit_game",0,"Quit");
	config_register_key(WINDOW_KEY_MAP,SDLK_m,"view_map",1,"Map");
	config_register_key(WINDOW_KEY_CROUCH,SDLK_LCTRL,"crouch",0,"Crouch");
	config_register_key(WINDOW_KEY_SNEAK,SDLK_v,"sneak",0,"Sneak");
	config_register_key(WINDOW_KEY_ENTER,SDLK_RETURN,"enter",0,"Enter");
	config_register_key(WINDOW_KEY_F1,SDLK_F1,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F2,SDLK_F2,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F3,SDLK_F3,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F4,SDLK_F4,NULL,0,NULL);
	config_register_key(WINDOW_KEY_YES,SDLK_y,NULL,0,NULL);
	config_register_key(WINDOW_KEY_YES,SDLK_z,NULL,0,NULL);
	config_register_key(WINDOW_KEY_NO,SDLK_n,NULL,0,NULL);
	config_register_key(WINDOW_KEY_VOLUME_UP,SDLK_KP_PLUS,"volume_up",0,"Volume up");
	config_register_key(WINDOW_KEY_VOLUME_DOWN,SDLK_KP_MINUS,"volume_down",0,"Volume down");
	config_register_key(WINDOW_KEY_V,SDLK_v,NULL,0,NULL);
	config_register_key(WINDOW_KEY_RELOAD,SDLK_r,"reload",0,"Reload");
	config_register_key(WINDOW_KEY_CHAT,SDLK_t,"chat_global",0,"Chat");
	config_register_key(WINDOW_KEY_FULLSCREEN,SDLK_F11,"fullscreen",0,"Fullscreen");
	config_register_key(WINDOW_KEY_SCREENSHOT,SDLK_F5,"screenshot",0,"Screenshot");
	config_register_key(WINDOW_KEY_CHANGETEAM,SDLK_COMMA,"change_team",0,"Team select");
	config_register_key(WINDOW_KEY_CHANGEWEAPON,SDLK_PERIOD,"change_weapon",0,"Gun select");
	config_register_key(WINDOW_KEY_PICKCOLOR,SDLK_e,"cube_color_sample",0,"Pick color");
	config_register_key(WINDOW_KEY_COMMAND,SDLK_SLASH,"chat_command",0,"Command");
	config_register_key(WINDOW_KEY_HIDEHUD,SDLK_F6,"hide_hud",1,"Hide HUD");
	config_register_key(WINDOW_KEY_LASTTOOL,SDLK_q,"last_tool",0,"Last tool");
	config_register_key(WINDOW_KEY_NETWORKSTATS,SDLK_F12,"network_stats",1,"Network stats");
	#endif

	#ifdef USE_GLFW
	config_register_key(WINDOW_KEY_UP,GLFW_KEY_W,"move_forward",0,"Forward");
	config_register_key(WINDOW_KEY_DOWN,GLFW_KEY_S,"move_backward",0,"Backward");
	config_register_key(WINDOW_KEY_LEFT,GLFW_KEY_A,"move_left",0,"Left");
	config_register_key(WINDOW_KEY_RIGHT,GLFW_KEY_D,"move_right",0,"Right");
	config_register_key(WINDOW_KEY_SPACE,GLFW_KEY_SPACE,"jump",0,"Jump");
	config_register_key(WINDOW_KEY_SPRINT,GLFW_KEY_LEFT_SHIFT,"sprint",0,"Sprint");
	config_register_key(WINDOW_KEY_CURSOR_UP,GLFW_KEY_UP,"cube_color_up",0,"Color up");
	config_register_key(WINDOW_KEY_CURSOR_DOWN,GLFW_KEY_DOWN,"cube_color_down",0,"Color down");
	config_register_key(WINDOW_KEY_CURSOR_LEFT,GLFW_KEY_LEFT,"cube_color_left",0,"Color left");
	config_register_key(WINDOW_KEY_CURSOR_RIGHT,GLFW_KEY_RIGHT,"cube_color_right",0,"Color right");
	config_register_key(WINDOW_KEY_BACKSPACE,GLFW_KEY_BACKSPACE,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL1,GLFW_KEY_1,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL2,GLFW_KEY_2,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL3,GLFW_KEY_3,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TOOL4,GLFW_KEY_4,NULL,0,NULL);
	config_register_key(WINDOW_KEY_TAB,GLFW_KEY_TAB,"view_score",0,"Score");
	config_register_key(WINDOW_KEY_ESCAPE,GLFW_KEY_ESCAPE,"quit_game",0,"Quit");
	config_register_key(WINDOW_KEY_MAP,GLFW_KEY_M,"view_map",1,"Map");
	config_register_key(WINDOW_KEY_CROUCH,GLFW_KEY_LEFT_CONTROL,"crouch",0,"Crouch");
	config_register_key(WINDOW_KEY_SNEAK,GLFW_KEY_V,"sneak",0,"Sneak");
	config_register_key(WINDOW_KEY_ENTER,GLFW_KEY_ENTER,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F1,GLFW_KEY_F1,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F2,GLFW_KEY_F2,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F3,GLFW_KEY_F3,NULL,0,NULL);
	config_register_key(WINDOW_KEY_F4,GLFW_KEY_F4,NULL,0,NULL);
	config_register_key(WINDOW_KEY_YES,GLFW_KEY_Y,NULL,0,NULL);
	config_register_key(WINDOW_KEY_YES,GLFW_KEY_Z,NULL,0,NULL);
	config_register_key(WINDOW_KEY_NO,GLFW_KEY_N,NULL,0,NULL);
	config_register_key(WINDOW_KEY_VOLUME_UP,GLFW_KEY_KP_ADD,"volume_up",0,"Volume up");
	config_register_key(WINDOW_KEY_VOLUME_DOWN,GLFW_KEY_KP_SUBTRACT,"volume_down",0,"Volume down");
	config_register_key(WINDOW_KEY_V,GLFW_KEY_V,NULL,0,NULL);
	config_register_key(WINDOW_KEY_RELOAD,GLFW_KEY_R,"reload",0,"Reload");
	config_register_key(WINDOW_KEY_CHAT,GLFW_KEY_T,"chat_global",0,"Chat");
	config_register_key(WINDOW_KEY_FULLSCREEN,GLFW_KEY_F11,"fullscreen",0,"Fullscreen");
	config_register_key(WINDOW_KEY_SCREENSHOT,GLFW_KEY_F5,"screenshot",0,"Screenshot");
	config_register_key(WINDOW_KEY_CHANGETEAM,GLFW_KEY_COMMA,"change_team",0,"Team select");
	config_register_key(WINDOW_KEY_CHANGEWEAPON,GLFW_KEY_PERIOD,"change_weapon",0,"Gun select");
	config_register_key(WINDOW_KEY_PICKCOLOR,GLFW_KEY_E,"cube_color_sample",0,"Pick color");
	config_register_key(WINDOW_KEY_COMMAND,GLFW_KEY_SLASH,"chat_command",0,"Command");
	config_register_key(WINDOW_KEY_HIDEHUD,GLFW_KEY_F6,"hide_hud",1,"Hide HUD");
	config_register_key(WINDOW_KEY_LASTTOOL,GLFW_KEY_Q,"last_tool",0,"Last tool");
	config_register_key(WINDOW_KEY_NETWORKSTATS,GLFW_KEY_F12,"network_stats",1,"Network stats");
	#endif

	ini_parse("config.ini",config_read_key,NULL);

	if(!list_created(&config_settings))
		list_create(&config_settings,sizeof(struct config_setting));
	else
		list_clear(&config_settings);

	list_add(&config_settings,&(struct config_setting){
		.value=settings_tmp.name,
		.type=CONFIG_TYPE_STRING,
		.max=sizeof(settings.name)-1,
		.name="Name",
		.help="ingame player name"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.mouse_sensitivity,
		.type=CONFIG_TYPE_FLOAT,
		.max=INT_MAX,
		.name="Mouse sensitivity"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.volume,
		.type=CONFIG_TYPE_INT,
		.max=10,
		.name="Volume"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.window_width,
		.type=CONFIG_TYPE_INT,
		.max=INT_MAX,
		.name="Game width",
		.defaults=640,800,854,1024,1280,1920,
		.defaults_length=6
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.window_height,
		.type=CONFIG_TYPE_INT,
		.max=INT_MAX,
		.name="Game height",
		.defaults=480,600,720,768,1024,1080,
		.defaults_length=6
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.vsync,
		.type=CONFIG_TYPE_INT,
		.max=INT_MAX,
		.name="V-Sync",
		.help="limits your game's fps",
		.defaults=0,1,60,120,240,
		.defaults_length=5
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.fullscreen,
		.type=CONFIG_TYPE_INT,
		.max=1,
		.name="Fullscreen"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.multisamples,
		.type=CONFIG_TYPE_INT,
		.max=16,
		.name="Multisamples",
		.help="smooth out block edges",
		.defaults=0,1,2,4,8,16,
		.defaults_length=6
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.voxlap_models,
		.type=CONFIG_TYPE_INT,
		.max=1,
		.help="Render models like in voxlap",
		.name="Voxlap models"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.greedy_meshing,
		.type=CONFIG_TYPE_INT,
		.max=1,
		.help="join similar mesh faces",
		.name="Greedy meshing"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.show_fps,
		.type=CONFIG_TYPE_INT,
		.max=1,
		.name="Show fps",
		.help="show your current fps and ping"
	});
	list_add(&config_settings,&(struct config_setting){
		.value=&settings_tmp.show_news,
		.type=CONFIG_TYPE_INT,
		.max=1,
		.name="Show news",
		.help="opens the bns news on exit"
	});
}
