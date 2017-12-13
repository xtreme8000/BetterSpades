#include "common.h"

struct RENDER_OPTIONS settings;

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
    }
    return 1;
}

void config_reload() {
    ini_parse("config.ini",config_read_key,NULL);
}
