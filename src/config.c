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
    }
    return 1;
}

void config_reload() {
    ini_parse("config.ini",config_read_key,NULL);
}
