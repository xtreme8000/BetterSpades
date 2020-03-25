/*
	Copyright (c) 2017-2020 ByteBit

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

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

extern int cameracontroller_bodyview_mode;
extern int cameracontroller_bodyview_player;
extern float cameracontroller_bodyview_zoom;

void cameracontroller_death_init(int player, float x, float y, float z);

void cameracontroller_fps(float dt);
void cameracontroller_spectator(float dt);
void cameracontroller_bodyview(float dt);
void cameracontroller_selection(float dt);
void cameracontroller_death(float dt);

void cameracontroller_fps_render(void);
void cameracontroller_spectator_render(void);
void cameracontroller_bodyview_render(void);
void cameracontroller_selection_render(void);
void cameracontroller_death_render(void);

#endif
