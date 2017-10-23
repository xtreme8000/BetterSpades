struct Grenade* grenade_add() {
    for(int k=0;k<GRENADES_MAX;k++) {
        if(!grenades[k].active) {
            grenades[k].active = 1;
            grenades[k].created = glfwGetTime();
            return &grenades[k];
        }
    }
    return &grenades[GRENADES_MAX-1];
}

int grenade_clipworld(int x, int y, int z) {
    int sz;

    if (x < 0 || x >= 512 || y < 0 || y >= 512)
        return 0;
    if (z < 0)
        return 0;
    sz = (int)z;
    if(sz == 63)
        sz=62;
    else if (sz >= 63)
        return 1;
    else if (sz < 0)
        return 0;
    return map_get((int)x,63-sz,(int)y)!=0xFFFFFFFF;
}

int grenade_move(struct Grenade* g, float dt) {
    float tmp;
    tmp = g->pos.z;
    g->pos.z = 63.0F-g->pos.y;
    g->pos.y = tmp;
    tmp = g->velocity.z;
    g->velocity.z = -g->velocity.y;
    g->velocity.y = tmp;

    struct Position fpos = g->pos; //old position

    //do velocity & gravity (friction is negligible)
    float f = dt*32.0F;
    g->velocity.z += dt;
    g->pos.x += g->velocity.x*f;
    g->pos.y += g->velocity.y*f;
    g->pos.z += g->velocity.z*f;
    //do rotation
    //FIX ME: Loses orientation after 45 degree bounce off wall
    // if(g->v.x > 0.1f || g->v.x < -0.1f || g->v.y > 0.1f || g->v.y < -0.1f)
    // {
        // f *= -0.5;
    // }
    //make it bounce (accurate)
    int ret = 0;

    int lpx = floor(g->pos.x);
    int lpy = floor(g->pos.y);
    int lpz = floor(g->pos.z);

    if(grenade_clipworld(lpx,lpy,lpz)) { //hit a wall

        ret = 1;
        if(fabs(g->velocity.x) > 0.1F ||
           fabs(g->velocity.y) > 0.1F ||
           fabs(g->velocity.z) > 0.1F)
            ret = 2; // play sound

        int lp2x = floor(fpos.x);
        int lp2y = floor(fpos.y);
        int lp2z = floor(fpos.z);
        if (lpz != lp2z && ((lpx == lp2x && lpy == lp2y) || !grenade_clipworld(lpx,lpy,lp2z)))
            g->velocity.z *= -1;
        else if(lpx != lp2x && ((lpy == lp2y && lpz == lp2z) || !grenade_clipworld(lp2x,lpy,lpz)))
            g->velocity.x *= -1;
        else if(lpy != lp2y && ((lpx == lp2x && lpz == lp2z) || !grenade_clipworld(lpx,lp2y,lpz)))
            g->velocity.y *= -1;
        g->pos = fpos; //set back to old position
        g->velocity.x *= 0.36F;
        g->velocity.y *= 0.36F;
        g->velocity.z *= 0.36F;
    }

    tmp = g->pos.y;
    g->pos.y = 63.0F-g->pos.z;
    g->pos.z = tmp;
    tmp = g->velocity.y;
    g->velocity.y = -g->velocity.z;
    g->velocity.z = tmp;

    return ret;
}


void grenade_update(float dt) {
    for(int k=0;k<GRENADES_MAX;k++) {
        if(grenades[k].active) {
            if(glfwGetTime()-grenades[k].created>grenades[k].fuse_length) {
                sound_createEx(NULL,SOUND_WORLD,
                    (grenades[k].pos.y<1.0F)?&sound_explode_water:&sound_explode,
                    grenades[k].pos.x,grenades[k].pos.y,grenades[k].pos.z,
                    grenades[k].velocity.x,grenades[k].velocity.y,grenades[k].velocity.z
                );
                grenades[k].active = 0;
            } else {
                if(grenade_move(&grenades[k],dt)==2) {
                    sound_createEx(NULL,SOUND_WORLD,
                        &sound_grenade_bounce,
                        grenades[k].pos.x,grenades[k].pos.y,grenades[k].pos.z,
                        grenades[k].velocity.x,grenades[k].velocity.y,grenades[k].velocity.z
                    );
                }
                //TODO: position grenade on ground properly
                glPushMatrix();
                glTranslatef(grenades[k].pos.x,grenades[k].pos.y+(model_grenade.zpiv+model_grenade.zsiz*2)*model_grenade.scale,grenades[k].pos.z);
                kv6_render(&model_grenade,TEAM_1);
                glPopMatrix();
            }
        }
    }
}
