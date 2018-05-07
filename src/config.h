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

extern struct RENDER_OPTIONS {
    char name[17];
	ALboolean opengl14;
	ALboolean color_correction;
	ALboolean shadow_entities;
	ALboolean ambient_occlusion;
	float render_distance;
	int window_width;
	int window_height;
	unsigned char multisamples;
	ALboolean player_arms;
	ALboolean fullscreen;
    ALboolean greedy_meshing;
    int vsync;
    float mouse_sensitivity;
} settings;

void config_reload(void);
