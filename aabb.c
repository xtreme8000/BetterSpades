void aabb_render(AABB* a) {
	glLineWidth(1.0F);
	glBegin(GL_LINES);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->min_z);
	glVertex3f(a->min_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->min_z);
	glVertex3f(a->max_x,a->max_y,a->min_z);
	glVertex3f(a->max_x,a->min_y,a->max_z);
	glVertex3f(a->max_x,a->max_y,a->max_z);
	glVertex3f(a->min_x,a->min_y,a->max_z);
	glVertex3f(a->min_x,a->max_y,a->max_z);
	glEnd();
}

void aabb_set_center(AABB* a, float x, float y, float z) {
	float size_x = a->max_x-a->min_x;
	float size_y = a->max_y-a->min_y;
	float size_z = a->max_z-a->min_z;
	a->min_x = x-size_x/2;
	a->min_y = y-size_y/2;
	a->min_z = z-size_z/2;
	a->max_x = x+size_x/2;
	a->max_y = y+size_y/2;
	a->max_z = z+size_z/2;
}

void aabb_set_size(AABB* a, float x, float y, float z) {
	a->max_x = a->min_x+x;
	a->max_y = a->min_y+y;
	a->max_z = a->min_z+z;
}

boolean aabb_intersection(AABB* a, AABB* b) {
	return (a->min_x <= b->max_x && b->min_x <= a->max_x) && (a->min_y <= b->max_y && b->min_y <= a->max_y) && (a->min_z <= b->max_z && b->min_z <= a->max_z);
}

boolean aabb_intersection_terrain(AABB* a) {
	AABB terrain_cube;

	int min_x = min(max(floor(a->min_x)-1,0),map_size_x);
	int min_y = min(max(floor(a->min_y)-1,0),map_size_y);
	int min_z = min(max(floor(a->min_z)-1,0),map_size_z);

	int max_x = min(max(ceil(a->max_x)+1,0),map_size_x);
	int max_y = min(max(ceil(a->max_y)+1,0),map_size_y);
	int max_z = min(max(ceil(a->max_z)+1,0),map_size_z);

	for(int x=min_x;x<max_x;x++) {
		for(int z=min_z;z<max_z;z++) {
			for(int y=min_y;y<max_y;y++) {
				if(map_colors[x+(y*map_size_z+z)*map_size_x]!=0xFFFFFFFF) {
					terrain_cube.min_x = x;
					terrain_cube.min_y = y;
					terrain_cube.min_z = z;
					terrain_cube.max_x = x+1;
					terrain_cube.max_y = y+1;
					terrain_cube.max_z = z+1;
					if(aabb_intersection(a,&terrain_cube)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}
