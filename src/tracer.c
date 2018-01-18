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

struct Tracer* tracers;

void tracer_add(unsigned char type, float x, float y, float z, float dx, float dy, float dz) {
    for(int k=0;k<TRACER_MAX;k++) {
        if(!tracers[k].used) {
            float spread = 0.0F;
            switch(type) {
                case WEAPON_RIFLE:
                    tracers[k].type = 0;
                    break;
                case WEAPON_SMG:
                    tracers[k].type = 1;
                    break;
                case WEAPON_SHOTGUN:
                    tracers[k].type = 2;
                    break;
            }
            tracers[k].r.origin.x = x+dx*3.0F;
            tracers[k].r.origin.y = y+dy*3.0F;
            tracers[k].r.origin.z = z+dz*3.0F;
            tracers[k].r.direction.x = dx;
            tracers[k].r.direction.y = dy;
            tracers[k].r.direction.z = dz;
            tracers[k].created = glfwGetTime();
            tracers[k].used = 1;
            break;
        }
    }
}

void tracer_render() {
    struct kv6_t* m[3] = {&model_semi_tracer,&model_smg_tracer,&model_shotgun_tracer};
    for(int k=0;k<TRACER_MAX;k++) {
        if(tracers[k].used) {
            matrix_push();
            matrix_translate(tracers[k].r.origin.x,tracers[k].r.origin.y,tracers[k].r.origin.z);
            matrix_pointAt(tracers[k].r.direction.x,tracers[k].r.direction.y,tracers[k].r.direction.z);
            matrix_rotate(90.0F,0.0F,1.0F,0.0F);
            matrix_upload();
            kv6_render(m[tracers[k].type],TEAM_SPECTATOR);
            matrix_pop();
        }
    }
}

void tracer_update(float dt) {
    for(int k=0;k<TRACER_MAX;k++) {
        if(tracers[k].used) {
            if(glfwGetTime()-tracers[k].created>0.5F) {
                tracers[k].used = 0;
            } else {
                struct Camera_HitType hit;
                camera_hit(&hit,-1,
                           tracers[k].r.origin.x,tracers[k].r.origin.y,tracers[k].r.origin.z,
                           tracers[k].r.direction.x,tracers[k].r.direction.y,tracers[k].r.direction.z,
                           256.0F*dt);
                if(hit.type==CAMERA_HITTYPE_BLOCK) {
                    sound_create(NULL,SOUND_WORLD,&sound_impact,tracers[k].r.origin.x,tracers[k].r.origin.y,tracers[k].r.origin.z);
                    tracers[k].used = 0;
                } else {
                    tracers[k].r.origin.x += tracers[k].r.direction.x*256.0F*dt;
                    tracers[k].r.origin.y += tracers[k].r.direction.y*256.0F*dt;
                    tracers[k].r.origin.z += tracers[k].r.direction.z*256.0F*dt;
                }
            }
        }
    }
}

void tracer_init() {
    tracers = calloc(sizeof(struct Tracer)*TRACER_MAX,1);
}
