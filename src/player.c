#include "common.h"

struct GameState gamestate;

unsigned char local_player_id = 0;
unsigned char local_player_health = 100;
unsigned char local_player_blocks = 50;
unsigned char local_player_grenades = 3;
unsigned char local_player_ammo, local_player_ammo_reserved;

int player_intersection_type = -1;
int player_intersection_player = 0;
float player_intersection_dist = 1024.0F;

struct Player players[PLAYERS_MAX];

#define FALL_DAMAGE_VELOCITY 0.58F
#define FALL_SLOW_DOWN 0.24F
#define SQRT 0.70710678F
#define WEAPON_PRIMARY 1
#define FALL_DAMAGE_SCALAR 4096

void player_reset(struct Player* p) {
    p->connected = 0;
    p->alive = 0;
    p->held_item = TOOL_GUN;
    p->block.red = 111;
    p->block.green = 111;
    p->block.blue = 111;
    p->physics.velocity.x = 0.0F;
    p->physics.velocity.y = 0.0F;
    p->physics.velocity.z = 0.0F;
    p->physics.jump = 0;
    p->physics.airborne = 0;
    p->physics.wade = 0;
    p->input.keys.packed = 0;
    p->input.buttons.packed = 0;
}

float player_swing_func(float x) {
    x -= (int)x;
    return (x<0.5F)?(x*4.0F-1.0F):(3.0F-x*4.0F);
}

float player_spade_func(float x) {
    return 1.0F-(x*5-(int)(x*5));
}

float player_tool_func(struct Player* p) {
    if(p->input.buttons.lmb) {
        switch(p->held_item) {
            case TOOL_SPADE:
                if(p==&players[local_player_id] && camera_mode==CAMERAMODE_FPS) {
                    return player_spade_func(glfwGetTime()-p->input.buttons.lmb_start)*90.0F;
                } else {
                    return (player_swing_func((glfwGetTime()-p->input.buttons.lmb_start)*5.0F)+1.0F)/2.0F*90.0F;
                }
            case TOOL_GRENADE:
                return max(-(glfwGetTime()-p->input.buttons.lmb_start)*35.0F,-35.0F);
        }
    }
    return 0.0F;
}

float player_tool_translate_func(struct Player* p) {
    if(p==&players[local_player_id]) {
        if(glfwGetTime()-p->item_showup<0.5F) {
            return 0.0F;
        }
        if(camera_mode==CAMERAMODE_FPS && p->held_item==TOOL_GUN && glfwGetTime()-weapon_last_shot<weapon_delay(players[local_player_id].weapon)) {
            return -(weapon_delay(players[local_player_id].weapon)-(glfwGetTime()-weapon_last_shot))/weapon_delay(players[local_player_id].weapon)*0.125F*(local_player_ammo>0);
        }
    }
    return 0.0F;
}

void player_update(float dt) {
    player_intersection_type = -1;
    player_intersection_dist = 1024.0F;
    for(int k=0;k<PLAYERS_MAX;k++) {
        if(players[k].connected && k!=local_player_id) {
            player_move(&players[k],dt);
            int intersections = player_render(&players[k],k);
            float d = (camera_x-players[k].pos.x)*(camera_x-players[k].pos.x)
                     +(camera_y-players[k].pos.y)*(camera_y-players[k].pos.y)
                     +(camera_z-players[k].pos.z)*(camera_z-players[k].pos.z);
            if(intersections && d<player_intersection_dist*player_intersection_dist) {
                player_intersection_dist = sqrt(d);
                player_intersection_player = k;
                player_intersection_type = -1;
                if(intersections & (1<<HITTYPE_ARMS)) {
                    player_intersection_type = HITTYPE_ARMS;
                }
                if(intersections & (1<<HITTYPE_LEGS)) {
                    player_intersection_type = HITTYPE_LEGS;
                }
                if(intersections & (1<<HITTYPE_TORSO)) {
                    player_intersection_type = HITTYPE_TORSO;
                }
                if(intersections & (1<<HITTYPE_HEAD)) {
                    player_intersection_type = HITTYPE_HEAD;
                }
            }

            if(players[k].alive && players[k].held_item==TOOL_GUN && players[k].input.buttons.lmb) {
                if(glfwGetTime()-players[k].gun_shoot_timer>weapon_delay(players[k].weapon)) {

                    sound_createEx(NULL,SOUND_WORLD,weapon_sound(players[k].weapon),
                                   players[k].gun_pos.x,players[k].gun_pos.y,players[k].gun_pos.z,
                                   players[k].physics.velocity.x,players[k].physics.velocity.y,players[k].physics.velocity.z
                               );
                    tracer_add(players[k].weapon,players[k].gun_pos.x,players[k].gun_pos.y,players[k].gun_pos.z,players[k].orientation.x,players[k].orientation.y,players[k].orientation.z);
                    players[k].gun_shoot_timer = glfwGetTime();
                }
            }
        }
    }
}

int player_render(struct Player* p, int id) {
    if(!p->alive) {
        matrix_push();
        matrix_translate(p->pos.x,p->pos.y,p->pos.z);
        matrix_pointAt(p->orientation.x,0.0F,p->orientation.z);
        matrix_rotate(90.0F,0.0F,1.0F,0.0F);
        matrix_upload();
        kv6_render(&model_playerdead,p->team);
        matrix_pop();
        return 0;
    }

    float time = glfwGetTime()*1000.0F;

    struct kv6_t* torso = p->input.keys.crouch?&model_playertorsoc:&model_playertorso;
    struct kv6_t* leg = p->input.keys.crouch?&model_playerlegc:&model_playerleg;
    float height = p->input.keys.crouch?0.8F:0.9F;


    float len = sqrt(pow(p->orientation.x,2.0F)+pow(p->orientation.z,2.0F));
    float fx = p->orientation.x/len;
    float fy = p->orientation.z/len;

    float a = (p->physics.velocity.x*fx+fy*p->physics.velocity.z)/(fx*fx+fy*fy);
    float b = (p->physics.velocity.z-fy*a)/fx;
    a /= 0.25F;
    b /= 0.25F;

    int intersections = 0;

    Ray ray;
    ray.origin.x = camera_x;
    ray.origin.y = camera_y;
    ray.origin.z = camera_z;
    ray.direction.x = sin(camera_rot_x)*sin(camera_rot_y);
    ray.direction.y = cos(camera_rot_y);
    ray.direction.z = cos(camera_rot_x)*sin(camera_rot_y);

    if(id!=local_player_id || !p->alive || camera_mode!=CAMERAMODE_FPS) {
        matrix_push();
        matrix_translate(p->physics.eye.x,p->physics.eye.y+height,p->physics.eye.z);
        matrix_pointAt(p->orientation.x,p->orientation.y,p->orientation.z);
        matrix_rotate(90.0F,0.0F,1.0F,0.0F);
        matrix_upload();
        kv6_render(&model_playerhead,p->team);
        if(kv6_intersection(&model_playerhead,ray)) {
            intersections |= (1<<HITTYPE_HEAD);
        }
        matrix_pop();

        matrix_push();
        matrix_translate(p->physics.eye.x,p->physics.eye.y+height,p->physics.eye.z-0.01F);
        matrix_pointAt(p->orientation.x,0.0F,p->orientation.z);
        matrix_rotate(90.0F,0.0F,1.0F,0.0F);
        matrix_upload();
        kv6_render(torso,p->team);
        if(kv6_intersection(torso,ray)) {
            intersections |= (1<<HITTYPE_TORSO);
        }
        matrix_pop();

        /*if(map.gamemode==PacketStateData.STATE_CTF) {
            mat4.translate(mov_matrix,mov_matrix,[0.0,-models.playerhead.zsiz*0.125-torso.zsiz*0.125*0.125,-torso.ysiz*0.125]);
            if(player.team==0 && "player" in map.gamestate.team_2_intel && map.players[map.gamestate.team_2_intel.player]==player) {
                models.intel.draw(0,0,0,0.125,1);
            }
            if(player.team==1 && "player" in map.gamestate.team_1_intel && map.players[map.gamestate.team_1_intel.player]==player) {
                models.intel.draw(0,0,0,0.125,0);
            }
        }*/

        matrix_push();
        matrix_translate(p->physics.eye.x,p->physics.eye.y+height,p->physics.eye.z);
        matrix_pointAt(p->orientation.x,0.0F,p->orientation.z);
        matrix_rotate(90.0F,0.0F,1.0F,0.0F);
        matrix_translate(torso->xsiz*0.1F*0.5F-leg->xsiz*0.1F*0.5F,-torso->zsiz*0.1F*(p->input.keys.crouch?0.6F:1.0F),p->input.keys.crouch?(-torso->zsiz*0.1F*0.75F):0.0F);
        matrix_rotate(45.0F*sin(time*DOUBLEPI/1000.0F)*a,1.0F,0.0F,0.0F);
        matrix_rotate(45.0F*sin(time*DOUBLEPI/1000.0F)*b,0.0F,0.0F,1.0F);
        matrix_upload();
        kv6_render(leg,p->team);
        if(kv6_intersection(leg,ray)) {
            intersections |= (1<<HITTYPE_LEGS);
        }
        matrix_pop();

        matrix_push();
        matrix_translate(p->physics.eye.x,p->physics.eye.y+height,p->physics.eye.z);
        matrix_pointAt(p->orientation.x,0.0F,p->orientation.z);
        matrix_rotate(90.0F,0.0F,1.0F,0.0F);
        matrix_translate(-torso->xsiz*0.1F*0.5F+leg->xsiz*0.1F*0.5F,-torso->zsiz*0.1F*(p->input.keys.crouch?0.6F:1.0F),p->input.keys.crouch?(-torso->zsiz*0.1F*0.75F):0.0F);
        matrix_rotate(-45.0F*sin(time*DOUBLEPI/1000.0F)*a,1.0F,0.0F,0.0F);
        matrix_rotate(-45.0F*sin(time*DOUBLEPI/1000.0F)*b,0.0F,0.0F,1.0F);
        matrix_upload();
        kv6_render(leg,p->team);
        if(kv6_intersection(leg,ray)) {
            intersections |= (1<<HITTYPE_LEGS);
        }
        matrix_pop();
    }

    matrix_push();
    matrix_translate(p->physics.eye.x,p->physics.eye.y+height,p->physics.eye.z);
    matrix_pointAt(p->orientation.x,p->orientation.y,p->orientation.z);
    matrix_rotate(90.0F,0.0F,1.0F,0.0F);
    if(id==local_player_id && camera_mode==CAMERAMODE_FPS) {
        matrix_translate(0.0F,-2*0.1F,-2*0.1F);
    } else {
        matrix_translate(0.0F,-2*0.1F,0*0.1F);
    }

    if(id==local_player_id && p->alive) {
        float speed = sqrt(pow(p->physics.velocity.x,2)+pow(p->physics.velocity.z,2))/0.25F;
        matrix_translate(0.0F,0.0F,0.1F*player_swing_func(time/1000.0F)*speed+player_tool_translate_func(p));
    }

    if(p->input.keys.sprint && !p->input.keys.crouch) {
        matrix_rotate(45.0F,1.0F,0.0F,0.0F);
    }
    /*if(p->input.buttons.lmb || p->input.buttons.rmb) {
        glRotatef(22.5F*player_spade_func(glfwGetTime()-p->input.buttons.lmb_start),1.0F,0.0F,0.0F);
    }*/
    if(id==local_player_id && glfwGetTime()-p->item_showup<0.5F) {
        matrix_rotate(45.0F-(glfwGetTime()-p->item_showup)*90.0F,1.0F,0.0F,0.0F);
    }
    if(p->held_item!=TOOL_SPADE || (p->held_item==TOOL_SPADE && id!=local_player_id)) {
        matrix_rotate(player_tool_func(p),1.0F,0.0F,0.0F);
    }
    if(id!=local_player_id || settings.player_arms || camera_mode!=CAMERAMODE_FPS) {
        matrix_upload();
        kv6_render(&model_playerarms,p->team);
        if(kv6_intersection(&model_playerarms,ray)) {
            intersections |= (1<<HITTYPE_ARMS);
        }
    }

    matrix_translate(-3.5F*0.1F+0.01F,0.0F,10*0.1F);
    if(p->held_item==TOOL_SPADE && id==local_player_id && glfwGetTime()-p->item_showup>=0.5F) {
        matrix_translate(0.0F,(model_spade.zpiv-model_spade.zsiz)*0.05F,0.0F);
        matrix_rotate(player_tool_func(p),1.0F,0.0F,0.0F);
        matrix_translate(0.0F,-(model_spade.zpiv-model_spade.zsiz)*0.05F,0.0F);
    }

    matrix_upload();
    switch(p->held_item) {
        case TOOL_SPADE:
            kv6_render(&model_spade,p->team);
            break;
        case TOOL_BLOCK:
            model_block.red = p->block.red/255.0F;
            model_block.green = p->block.green/255.0F;
            model_block.blue = p->block.blue/255.0F;
            kv6_render(&model_block,p->team);
            break;
        case TOOL_GUN:
            if(!(camera_mode==CAMERAMODE_FPS && players[local_player_id].input.buttons.rmb)) {
                switch(p->weapon) {
                    case WEAPON_RIFLE:
                        kv6_render(&model_semi,p->team);
                        break;
                    case WEAPON_SMG:
                        kv6_render(&model_smg,p->team);
                        break;
                    case WEAPON_SHOTGUN:
                        kv6_render(&model_shotgun,p->team);
                        break;
                }
            }
            break;
        case TOOL_GRENADE:
            kv6_render(&model_grenade,p->team);
            break;
    }
    float v[4] = {0,0,0,1};
    matrix_vector(v);
    float v2[4] = {1,0,0,1};
    matrix_vector(v2);
    float v3[4] = {0,0,1,1};
    matrix_vector(v3);

    p->gun_pos.x = v[0];
    p->gun_pos.y = v[1];
    p->gun_pos.z = v[2];

    matrix_pop();
    matrix_identity();
    matrix_upload();
    glDisable(GL_DEPTH_TEST);
    glBegin(GL_LINES);
    glVertex3f(v[0],v[1],v[2]);
    glVertex3f(v2[0],v2[1],v2[2]);
    glVertex3f(v[0],v[1],v[2]);
    glVertex3f(v3[0],v3[1],v3[2]);
    glEnd();
    glEnable(GL_DEPTH_TEST);

    return intersections;
}

int player_clipbox(float x, float y, float z) {
    int sz;

    if (x < 0 || x >= 512 || y < 0 || y >= 512)
        return 1;
    else if (z < 0)
        return 0;
    sz = (int)z;
    if(sz == 63)
        sz=62;
    else if (sz >= 64)
        return 1;
    return map_get((int)x,63-sz,(int)y)!=0xFFFFFFFF;
}

void player_reposition(struct Player* p)  {
    p->physics.eye.x = p->pos.x;
    p->physics.eye.y = p->pos.y;
    p->physics.eye.z = p->pos.z;
    float f = p->physics.lastclimb-glfwGetTime();
    if(f>-0.25F) {
        p->physics.eye.z += (f+0.25F)/0.25F;
    }
}


void player_coordsystem_adjust1(struct Player* p) {
    float tmp;

    tmp = p->pos.z;
    p->pos.z = 63.0F-p->pos.y;
    p->pos.y = tmp;

    tmp = p->physics.eye.z;
    p->physics.eye.z = 63.0F-p->physics.eye.y;
    p->physics.eye.y = tmp;

    tmp = p->physics.velocity.z;
    p->physics.velocity.z = -p->physics.velocity.y;
    p->physics.velocity.y = tmp;

    tmp = p->orientation.z;
    p->orientation.z = -p->orientation.y;
    p->orientation.y = tmp;
}

void player_coordsystem_adjust2(struct Player* p) {
    float tmp;

    tmp = p->pos.y;
    p->pos.y = 63.0F-p->pos.z;
    p->pos.z = tmp;

    tmp = p->physics.eye.y;
    p->physics.eye.y = 63.0F-p->physics.eye.z;
    p->physics.eye.z = tmp;

    tmp = p->physics.velocity.y;
    p->physics.velocity.y = -p->physics.velocity.z;
    p->physics.velocity.z = tmp;

    tmp = p->orientation.y;
    p->orientation.y = -p->orientation.z;
    p->orientation.z = tmp;
}

void player_boxclipmove(struct Player* p, float fsynctics) {
        float offset, m, f, nx, ny, nz, z;
    	long climb = 0;

    	f = fsynctics*32.f;
    	nx = f*p->physics.velocity.x+p->pos.x;
    	ny = f*p->physics.velocity.y+p->pos.y;

    	if(p->input.keys.crouch)
    	{
    		offset = 0.45f;
    		m = 0.9f;
    	}
    	else
    	{
    		offset = 0.9f;
    		m = 1.35f;
    	}

    	nz = p->pos.z + offset;

    	if(p->physics.velocity.x < 0) f = -0.45f;
    	else f = 0.45f;
    	z=m;
    	while(z>=-1.36f && !player_clipbox(nx+f, p->pos.y-0.45f, nz+z) && !player_clipbox(nx+f, p->pos.y+0.45f, nz+z))
    		z-=0.9f;
    	if(z<-1.36f) p->pos.x = nx;
    	else if(!p->input.keys.crouch && p->orientation.z<0.5f && !p->input.keys.sprint)
    	{
    		z=0.35f;
    		while(z>=-2.36f && !player_clipbox(nx+f, p->pos.y-0.45f, nz+z) && !player_clipbox(nx+f, p->pos.y+0.45f, nz+z))
    			z-=0.9f;
    		if(z<-2.36f)
    		{
    			p->pos.x = nx;
    			climb=1;
    		}
    		else p->physics.velocity.x = 0;
    	}
    	else p->physics.velocity.x = 0;

    	if(p->physics.velocity.y < 0) f = -0.45f;
    	else f = 0.45f;
    	z=m;
    	while(z>=-1.36f && !player_clipbox(p->pos.x-0.45f, ny+f, nz+z) && !player_clipbox(p->pos.x+0.45f, ny+f, nz+z))
    		z-=0.9f;
    	if(z<-1.36f) p->pos.y = ny;
    	else if(!p->input.keys.crouch && p->orientation.z<0.5f && !p->input.keys.sprint && !climb)
    	{
    		z=0.35f;
    		while(z>=-2.36f && !player_clipbox(p->pos.x-0.45f, ny+f, nz+z) && !player_clipbox(p->pos.x+0.45f, ny+f, nz+z))
    			z-=0.9f;
    		if(z<-2.36f)
    		{
    			p->pos.y = ny;
    			climb=1;
    		}
    		else p->physics.velocity.y = 0;
    	}
    	else if(!climb)
    		p->physics.velocity.y = 0;

    	if(climb)
    	{
    		p->physics.velocity.x *= 0.5f;
    		p->physics.velocity.y *= 0.5f;
    		p->physics.lastclimb = glfwGetTime();
    		nz--;
    		m = -1.35f;
    	}
    	else
    	{
    		if(p->physics.velocity.z < 0)
    			m=-m;
    		nz += p->physics.velocity.z*fsynctics*32.f;
    	}

    	p->physics.airborne = 1;

    	if(player_clipbox(p->pos.x-0.45f, p->pos.y-0.45f, nz+m) ||
    		player_clipbox(p->pos.x-0.45f, p->pos.y+0.45f, nz+m) ||
    		player_clipbox(p->pos.x+0.45f, p->pos.y-0.45f, nz+m) ||
    		player_clipbox(p->pos.x+0.45f, p->pos.y+0.45f, nz+m))
    	{
    		if(p->physics.velocity.z >= 0)
    		{
    			p->physics.wade = p->pos.z > 61;
    			p->physics.airborne = 0;
    		}
    		p->physics.velocity.z = 0;
    	}
    	else
    		p->pos.z = nz-offset;

    player_reposition(p);
}

int player_move(struct Player* p, float fsynctics) {
    if(!p->alive) {
        p->physics.velocity.y -= fsynctics;
        if(map_get(p->pos.x+p->physics.velocity.x*fsynctics*32.0F,p->pos.y+p->physics.velocity.y*fsynctics*32.0F,p->pos.z+p->physics.velocity.z*fsynctics*32.0F)==0xFFFFFFFF) {
            p->pos.x += p->physics.velocity.x*fsynctics*32.0F;
            p->pos.y += p->physics.velocity.y*fsynctics*32.0F;
            p->pos.z += p->physics.velocity.z*fsynctics*32.0F;
        } else {
            p->physics.velocity.x *= 0.36F;
            p->physics.velocity.y *= -0.36F;
            p->physics.velocity.z *= 0.36F;
        }
        return 0;
    }

    unsigned char local = (p==&players[local_player_id] && camera_mode==CAMERAMODE_FPS);

    player_coordsystem_adjust1(p);
    float f, f2;

	//move player and perform simple physics (gravity, momentum, friction)
	if(p->physics.jump) {
        sound_create(NULL,local?SOUND_LOCAL:SOUND_WORLD,p->physics.wade?&sound_jump_water:&sound_jump,p->pos.x,63.0F-p->pos.z,p->pos.y);
		p->physics.jump = 0;
		p->physics.velocity.z = -0.36f;
	}

	f = fsynctics; //player acceleration scalar
	if(p->physics.airborne)
		f *= 0.1f;
	else if(p->input.keys.crouch)
		f *= 0.3f;
	else if((p->input.buttons.rmb && p->weapon == WEAPON_PRIMARY) || p->input.keys.sneak)
		f *= 0.5f;
	else if(p->input.keys.sprint)
		f *= 1.3f;

	if((p->input.keys.up || p->input.keys.down) && (p->input.keys.left || p->input.keys.right))
		f *= SQRT; //if strafe + forward/backwards then limit diagonal velocity

    float len = sqrt(pow(p->orientation.x,2.0F)+pow(p->orientation.y,2.0F));
    float sx = -p->orientation.y/len;
    float sy = p->orientation.x/len;

	if(p->input.keys.up)
	{
		p->physics.velocity.x += p->orientation.x*f;
		p->physics.velocity.y += p->orientation.y*f;
	}
	else if(p->input.keys.down)
	{
		p->physics.velocity.x -= p->orientation.x*f;
		p->physics.velocity.y -= p->orientation.y*f;
	}
	if(p->input.keys.left)
	{
		p->physics.velocity.x -= sx*f;
		p->physics.velocity.y -= sy*f;
	}
	else if(p->input.keys.right)
	{
		p->physics.velocity.x += sx*f;
		p->physics.velocity.y += sy*f;
	}

	f = fsynctics + 1;
	p->physics.velocity.z += fsynctics;
	p->physics.velocity.z /= f; //air friction
	if(p->physics.wade)
		f = fsynctics*6.0F + 1; //water friction
	else if(!p->physics.airborne)
		f = fsynctics*4.0F + 1; //ground friction
	p->physics.velocity.x /= f;
	p->physics.velocity.y /= f;
	f2 = p->physics.velocity.z;
	player_boxclipmove(p,fsynctics);
	//hit ground... check if hurt

    int ret = 0;

	if(!p->physics.velocity.z && (f2 > FALL_SLOW_DOWN))
	{
		//slow down on landing
		p->physics.velocity.x *= 0.5F;
		p->physics.velocity.y *= 0.5F;

		//return fall damage
		if(f2 > FALL_DAMAGE_VELOCITY)
		{
			f2 -= FALL_DAMAGE_VELOCITY;
			ret = f2*f2*FALL_DAMAGE_SCALAR;
            sound_create(NULL,local?SOUND_LOCAL:SOUND_WORLD,&sound_hurt_fall,p->pos.x,63.0F-p->pos.z,p->pos.y);
		} else {
            sound_create(NULL,local?SOUND_LOCAL:SOUND_WORLD,p->physics.wade?&sound_land_water:&sound_land,p->pos.x,63.0F-p->pos.z,p->pos.y);
            ret = -1;
        }
	}

    player_coordsystem_adjust2(p);

    if(sqrt(pow(p->physics.velocity.x,2.0F)+pow(p->physics.velocity.z,2.0F))>0.125F
        && !p->physics.airborne
        && (p->input.keys.up || p->input.keys.down || p->input.keys.left || p->input.keys.right)
        && glfwGetTime()-p->sound.feet_started>(p->input.keys.sprint?(0.5F/1.3F):0.5F)) {

        struct Sound_wav* footstep[8] = {&sound_footstep1,&sound_footstep2,&sound_footstep3,&sound_footstep4,
                                         &sound_wade1,&sound_wade2,&sound_wade3,&sound_wade4};
        sound_create(&p->sound.feet,local?SOUND_LOCAL:SOUND_WORLD,footstep[(rand()%4)+(p->physics.wade?4:0)],p->pos.x,p->pos.y,p->pos.z);
        p->sound.feet_started = glfwGetTime();
    }

    sound_position(&p->sound.feet,p->pos.x,p->pos.y,p->pos.z);
    sound_velocity(&p->sound.feet,p->physics.velocity.x,p->physics.velocity.y,p->physics.velocity.z);

    return ret;
}
