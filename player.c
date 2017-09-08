#define FALL_DAMAGE_VELOCITY 0.58F
#define FALL_SLOW_DOWN 0.24F
#define SQRT 0.70710678F
#define WEAPON_PRIMARY 1
#define FALL_DAMAGE_SCALAR 4096

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
    player_coordsystem_adjust1(p);
    float f, f2;

	//move player and perform simple physics (gravity, momentum, friction)
	if(p->input.keys.jump)
	{
		p->input.keys.jump = 0;
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
		} else {
            ret = -1;
        }
	}

    player_coordsystem_adjust2(p);
    return ret;
}
