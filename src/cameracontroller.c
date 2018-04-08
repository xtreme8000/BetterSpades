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

int cameracontroller_bodyview_player = 0;

float lx,ly,lz;
void cameracontroller_fps(float dt) {
    players[local_player_id].connected = 1;
	players[local_player_id].alive = 1;

    char cooldown = 0;
    if(players[local_player_id].held_item==TOOL_GRENADE && local_player_grenades==0) {
        players[local_player_id].held_item--;
        cooldown = 1;
    }
    if(players[local_player_id].held_item==TOOL_GUN && local_player_ammo+local_player_ammo_reserved==0) {
        players[local_player_id].held_item--;
        cooldown = 1;
    }
    if(players[local_player_id].held_item==TOOL_BLOCK && local_player_blocks==0) {
        players[local_player_id].held_item--;
        cooldown = 1;
    }

    if(cooldown) {
        players[local_player_id].item_disabled = glfwGetTime();
		players[local_player_id].items_show_start = glfwGetTime();
		players[local_player_id].items_show = 1;
    }

    if(chat_input_mode==CHAT_NO_INPUT) {
    	players[local_player_id].input.keys.up = key_map[GLFW_KEY_W];
    	players[local_player_id].input.keys.down = key_map[GLFW_KEY_S];
    	players[local_player_id].input.keys.left = key_map[GLFW_KEY_A];
    	players[local_player_id].input.keys.right = key_map[GLFW_KEY_D];
        if(players[local_player_id].input.keys.crouch && !key_map[GLFW_KEY_LEFT_CONTROL] && player_uncrouch(&players[local_player_id])) {
            players[local_player_id].input.keys.crouch = 0;
        }
        if(key_map[GLFW_KEY_LEFT_CONTROL]) {
            players[local_player_id].input.keys.crouch = 1;
        }
    	//players[local_player_id].input.keys.crouch = key_map[GLFW_KEY_LEFT_CONTROL];
    	players[local_player_id].input.keys.sprint = key_map[GLFW_KEY_LEFT_SHIFT];
        players[local_player_id].input.keys.jump = key_map[GLFW_KEY_SPACE];
        players[local_player_id].input.keys.sneak = key_map[GLFW_KEY_V];
    }

    camera_x = players[local_player_id].physics.eye.x;
    camera_y = players[local_player_id].physics.eye.y+player_height(&players[local_player_id]);
    camera_z = players[local_player_id].physics.eye.z;

    if(key_map[GLFW_KEY_LEFT_SHIFT]) {
        players[local_player_id].item_disabled = glfwGetTime();
    } else {
        if(glfwGetTime()-players[local_player_id].item_disabled<0.4F && !players[local_player_id].items_show) {
            players[local_player_id].items_show_start = glfwGetTime();
            players[local_player_id].items_show = 1;
        }
    }

    players[local_player_id].input.buttons.lmb = button_map[0];
    if(players[local_player_id].held_item!=TOOL_GUN) {
        players[local_player_id].input.buttons.rmb = button_map[1];
    }

    if(key_map[GLFW_KEY_SPACE] && !players[local_player_id].physics.airborne) {
        players[local_player_id].physics.jump = 1;
    }

    if(chat_input_mode!=CHAT_NO_INPUT) {
        players[local_player_id].input.keys.packed = 0;
        players[local_player_id].input.buttons.packed = 0;
    }

	lx = lx*pow(0.7F,dt*60.0F)+(sin(camera_rot_x)*sin(camera_rot_y))*pow(0.3F,dt*60.0F);
	ly = ly*pow(0.7F,dt*60.0F)+(cos(camera_rot_y))*pow(0.3F,dt*60.0F);
	lz = lz*pow(0.7F,dt*60.0F)+(cos(camera_rot_x)*sin(camera_rot_y))*pow(0.3F,dt*60.0F);

    float len = sqrt(lx*lx+ly*ly+lz*lz);
    players[local_player_id].orientation.x = lx/len;
	players[local_player_id].orientation.y = ly/len;
	players[local_player_id].orientation.z = lz/len;

    camera_vx = players[local_player_id].physics.velocity.x;
    camera_vy = players[local_player_id].physics.velocity.y;
    camera_vz = players[local_player_id].physics.velocity.z;

    matrix_lookAt(camera_x,camera_y,camera_z,camera_x+sin(camera_rot_x)*sin(camera_rot_y),camera_y+cos(camera_rot_y),camera_z+cos(camera_rot_x)*sin(camera_rot_y),0.0F,1.0F,0.0F);
}

void cameracontroller_spectator(float dt) {
    AABB camera = {0};
    aabb_set_size(&camera,camera_size,camera_height,camera_size);
    aabb_set_center(&camera,camera_x,camera_y-camera_eye_height,camera_z);

    float x = 0.0F, y = 0.0F, z = 0.0F;

	if(key_map[GLFW_KEY_W]) {
		x += sin(camera_rot_x)*sin(camera_rot_y);
		y += cos(camera_rot_y);
		z += cos(camera_rot_x)*sin(camera_rot_y);
	} else {
		if(key_map[GLFW_KEY_S]) {
			x -= sin(camera_rot_x)*sin(camera_rot_y);
			y -= cos(camera_rot_y);
			z -= cos(camera_rot_x)*sin(camera_rot_y);
		}
	}

	if(key_map[GLFW_KEY_A]) {
		x += sin(camera_rot_x+1.57);
		z += cos(camera_rot_x+1.57);
	} else {
		if(key_map[GLFW_KEY_D]) {
			x += sin(camera_rot_x-1.57);
			z += cos(camera_rot_x-1.57);
		}
	}

	if(key_map[GLFW_KEY_SPACE]) {
		y++;
	} else {
		if(key_map[GLFW_KEY_LEFT_CONTROL]) {
			y--;
		}
	}

	float len = sqrt(x*x+y*y+z*z);
	if(len>0.0F) {
		camera_movement_x = (x/len)*camera_speed*dt;
		camera_movement_y = (y/len)*camera_speed*dt;
		camera_movement_z = (z/len)*camera_speed*dt;
	}

	if(abs(camera_movement_x)<1.0F) {
		camera_movement_x *= pow(0.0025F,dt);
	}
	if(abs(camera_movement_y)<1.0F) {
		camera_movement_y *= pow(0.0025F,dt);
	}
	if(abs(camera_movement_z)<1.0F) {
		camera_movement_z *= pow(0.0025F,dt);
	}

	aabb_set_center(&camera,camera_x+camera_movement_x,camera_y-camera_eye_height,camera_z);

	if(camera_x+camera_movement_x<0 || camera_x+camera_movement_x>map_size_x || aabb_intersection_terrain(&camera)) {
		camera_movement_x = 0.0F;
	}

	aabb_set_center(&camera,camera_x+camera_movement_x,camera_y+camera_movement_y-camera_eye_height,camera_z);
	if(camera_y+camera_movement_y<0 || aabb_intersection_terrain(&camera)) {
		camera_movement_y = 0.0F;
	}

	aabb_set_center(&camera,camera_x+camera_movement_x,camera_y+camera_movement_y-camera_eye_height,camera_z+camera_movement_z);
	if(camera_z+camera_movement_z<0 || camera_z+camera_movement_z>map_size_z || aabb_intersection_terrain(&camera)) {
		camera_movement_z = 0.0F;
	}

	camera_x += camera_movement_x;
	camera_y += camera_movement_y;
	camera_z += camera_movement_z;
    camera_vx = camera_movement_x;
    camera_vy = camera_movement_y;
    camera_vz = camera_movement_z;
    matrix_lookAt(camera_x,camera_y,camera_z,camera_x+sin(camera_rot_x)*sin(camera_rot_y),camera_y+cos(camera_rot_y),camera_z+cos(camera_rot_x)*sin(camera_rot_y),0.0F,1.0F,0.0F);
}

void cameracontroller_bodyview(float dt) {
    //check if we cant spectate the player anymore
    for(int k=0;k<PLAYERS_MAX*2;k++) { //a while(1) loop caused it to get stuck on map change when playing on babel
        if(player_can_spectate(&players[cameracontroller_bodyview_player]))
            break;
        cameracontroller_bodyview_player = (cameracontroller_bodyview_player+1)%PLAYERS_MAX;
    }

    AABB camera;
    aabb_set_size(&camera,0.4F,0.4F,0.4F);

    float k;
    float traverse_lengths[2] = {-1,-1};
    for(k=0.0F;k<5.0F;k+=0.05F) {
        aabb_set_center(&camera,
            players[cameracontroller_bodyview_player].pos.x-sin(camera_rot_x)*sin(camera_rot_y)*k,
            players[cameracontroller_bodyview_player].pos.y-cos(camera_rot_y)*k+player_height2(&players[cameracontroller_bodyview_player]),
            players[cameracontroller_bodyview_player].pos.z-cos(camera_rot_x)*sin(camera_rot_y)*k
        );
        if(aabb_intersection_terrain(&camera) && traverse_lengths[0]<0) {
            traverse_lengths[0] = max(k-0.1F,0);
        }
        aabb_set_center(&camera,
            players[cameracontroller_bodyview_player].pos.x+sin(camera_rot_x)*sin(camera_rot_y)*k,
            players[cameracontroller_bodyview_player].pos.y+cos(camera_rot_y)*k+player_height2(&players[cameracontroller_bodyview_player]),
            players[cameracontroller_bodyview_player].pos.z+cos(camera_rot_x)*sin(camera_rot_y)*k
        );
        if(!aabb_intersection_terrain(&camera) && traverse_lengths[1]<0) {
            traverse_lengths[1] = max(k-0.1F,0);
        }
    }
    if(traverse_lengths[0]<0)
        traverse_lengths[0] = 5.0F;
    if(traverse_lengths[1]<0)
        traverse_lengths[1] = 5.0F;

    k = (traverse_lengths[0]<=0)?(-traverse_lengths[1]):traverse_lengths[0];

    //this is needed to determine which chunks need/can be rendered and for sound, minimap etc...
    camera_x = players[cameracontroller_bodyview_player].pos.x-sin(camera_rot_x)*sin(camera_rot_y)*k;
    camera_y = players[cameracontroller_bodyview_player].pos.y-cos(camera_rot_y)*k+player_height2(&players[cameracontroller_bodyview_player]);
    camera_z = players[cameracontroller_bodyview_player].pos.z-cos(camera_rot_x)*sin(camera_rot_y)*k;
    camera_vx = players[cameracontroller_bodyview_player].physics.velocity.x;
    camera_vy = players[cameracontroller_bodyview_player].physics.velocity.y;
    camera_vz = players[cameracontroller_bodyview_player].physics.velocity.z;

    matrix_lookAt(players[cameracontroller_bodyview_player].pos.x-sin(camera_rot_x)*sin(camera_rot_y)*k,
              players[cameracontroller_bodyview_player].pos.y-cos(camera_rot_y)*k+player_height2(&players[cameracontroller_bodyview_player]),
              players[cameracontroller_bodyview_player].pos.z-cos(camera_rot_x)*sin(camera_rot_y)*k,
              players[cameracontroller_bodyview_player].pos.x,
              players[cameracontroller_bodyview_player].pos.y+player_height2(&players[cameracontroller_bodyview_player]),
              players[cameracontroller_bodyview_player].pos.z,
              0.0F,1.0F,0.0F);
}

void cameracontroller_selection(float dt) {
    camera_x = 256.0F;
    camera_y = 125.0F;
    camera_z = 256.0F;
    camera_vx = 0.0F;
    camera_vy = 0.0F;
    camera_vz = 0.0F;
    matrix_rotate(90.0F,1.0F,0.0F,0.0F);
    matrix_translate(-camera_x,-camera_y,-camera_z);
}
