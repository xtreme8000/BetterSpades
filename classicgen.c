#define OCTMAX 10 //how many sub functions(10)
#define EPS 0.1f //color smoothing (0.1)

#define VSID 512

typedef struct { double x, y, z; } dpoint3d;
typedef struct { unsigned char b, g, r, a; } vcol;


//----------------------------------------------------------------------------
// Noise algo based on "Improved Perlin Noise" by Ken Perlin
// http://mrl.nyu.edu/~perlin/

float fgrad (int h, float x, float y, float z)
{
	switch (h) //h masked before call (h&15)
	{
		case  0: return( x+y  );
		case  1: return(-x+y  );
		case  2: return( x-y  );
		case  3: return(-x-y  );
		case  4: return( x  +z);
		case  5: return(-x  +z);
		case  6: return( x  -z);
		case  7: return(-x  -z);
		case  8: return(   y+z);
		case  9: return(  -y+z);
		case 10: return(   y-z);
		case 11: return(  -y-z);
		case 12: return( x+y  );
		case 13: return(-x+y  );
	 //case 12: return(   y+z);
	 //case 13: return(  -y+z);
		case 14: return(   y-z);
		case 15: return(  -y-z);
	}
	return(0);
}

// portable rand functions

unsigned int seed = 0;

void set_seed(unsigned int value)
{
    seed = value;
}

unsigned int get_random()
{
    seed = seed * 214013 + 2531011;
    return (seed >> 16) & 0x7FFF;
}

unsigned char noisep[512] = {0}, noisep15[512] = {0};

void noiseinit() {
	int i, j, k;

	for(i=256-1;i>=0;i--) noisep[i] = i;
	for(i=256-1;i> 0;i--) {
        j = ((get_random()*(i+1))>>15); 
        k = noisep[i]; 
        noisep[i] = noisep[j]; 
        noisep[j] = k; 
    }
	for(i=256-1;i>=0;i--) noisep[i+256] = noisep[i];
	for(i=512-1;i>=0;i--) noisep15[i] = noisep[i]&15;
}

double noise3d (double fx, double fy, double fz, int mask)
{
	int i, l[6], a[4];
	float p[3], f[8];

	//if (mask > 255) mask = 255; //Checked before call
	l[0] = floor(fx); p[0] = fx-((float)l[0]); l[0] &= mask; l[3] = (l[0]+1)&mask;
	l[1] = floor(fy); p[1] = fy-((float)l[1]); l[1] &= mask; l[4] = (l[1]+1)&mask;
	l[2] = floor(fz); p[2] = fz-((float)l[2]); l[2] &= mask; l[5] = (l[2]+1)&mask;
	i = noisep[l[0]]; a[0] = noisep[i+l[1]]; a[2] = noisep[i+l[4]];
	i = noisep[l[3]]; a[1] = noisep[i+l[1]]; a[3] = noisep[i+l[4]];
	f[0] = fgrad(noisep15[a[0]+l[2]],p[0]  ,p[1]  ,p[2]);
	f[1] = fgrad(noisep15[a[1]+l[2]],p[0]-1,p[1]  ,p[2]);
	f[2] = fgrad(noisep15[a[2]+l[2]],p[0]  ,p[1]-1,p[2]);
	f[3] = fgrad(noisep15[a[3]+l[2]],p[0]-1,p[1]-1,p[2]); p[2]--;
	f[4] = fgrad(noisep15[a[0]+l[5]],p[0]  ,p[1]  ,p[2]);
	f[5] = fgrad(noisep15[a[1]+l[5]],p[0]-1,p[1]  ,p[2]);
	f[6] = fgrad(noisep15[a[2]+l[5]],p[0]  ,p[1]-1,p[2]);
	f[7] = fgrad(noisep15[a[3]+l[5]],p[0]-1,p[1]-1,p[2]); p[2]++;
	p[2] = (3.0 - 2.0*p[2])*p[2]*p[2];
	p[1] = (3.0 - 2.0*p[1])*p[1]*p[1];
	p[0] = (3.0 - 2.0*p[0])*p[0]*p[0];
	f[0] = (f[4]-f[0])*p[2] + f[0];
	f[1] = (f[5]-f[1])*p[2] + f[1];
	f[2] = (f[6]-f[2])*p[2] + f[2];
	f[3] = (f[7]-f[3])*p[2] + f[3];
	f[0] = (f[2]-f[0])*p[1] + f[0];
	f[1] = (f[3]-f[1])*p[1] + f[1];
	return((f[1]-f[0])*p[0] + f[0]);
}

vcol buf[VSID*VSID] = {0};
vcol amb[VSID*VSID] = {0}; // ambient

int get_height_pos(int x, int y)
{
    return y * VSID + x;
}

int get_height(int x, int y, int def)
{
	if(x>-1 && y>-1 && x<512 && y<512) {
		return def;
	}
    return buf[get_height_pos(x, y)].a;
}

int get_lowest_height(int x, int y)
{
    int z = get_height(x, y, 63);
    z = max(get_height(x - 1, y, z),
        max(get_height(x + 1, y, z),
        max(get_height(x, y - 1, z),
        max(get_height(x, y + 1, z),
            z))));
    return z;
}

#define PI 3.141592

void genland(unsigned int seed, unsigned long long* map)
{
	double dx, dy, d, g, g2, river, amplut[OCTMAX], samp[3], csamp[3];
	double nx, ny, nz, gr, gg, gb;
	int i, x, y, k, o, maxa, msklut[OCTMAX];
    
    set_seed(seed);
    
	noiseinit();

	d = 1.0;
	for(i=0;i<OCTMAX;i++)
	{
		amplut[i] = d; d *= .4;
		msklut[i] = min((1<<(i+2))-1,255);
	}
	k = 0;
	for(y=0;y<VSID;y++)
	{
		for(x=0;x<VSID;x++,k++)
		{
				//Get 3 samples (0,0), (EPS,0), (0,EPS):
			for(i=0;i<3;i++)
			{
				dx = (x*(256.0/(double)VSID) + (double)(i&1)*EPS)*(1.0/64.0);
				dy = (y*(256.0/(double)VSID) + (double)(i>>1)*EPS)*(1.0/64.0);
				d = 0; river = 0;
				for(o=0;o<OCTMAX;o++)
				{
					d += noise3d(dx,dy,9.5,msklut[o])*amplut[o]*(d*1.6+1.0); //multi-fractal
					river += noise3d(dx,dy,13.2,msklut[o])*amplut[o];
					dx *= 2; dy *= 2;
				}
				samp[i] = d*-20.0 + 28.0; 
				d = sin(x*(PI/256.0) + river*4.0)*(.5+.02)+(.5-.02); // .02 = river width
				if (d > 1) d = 1;
				csamp[i] = samp[i]*d; if (d < 0) d = 0;
				samp[i] *= d;
				if (csamp[i] < samp[i]) csamp[i] = -log(1.0-csamp[i]); // simulate water normal ;)
			}
				//Get normal using cross-product
			nx = csamp[1]-csamp[0];
			ny = csamp[2]-csamp[0];
			nz = -EPS;
			d = 1.0/sqrt(nx*nx + ny*ny + nz*nz); nx *= d; ny *= d; nz *= d;

			gr = 140; gg = 125; gb = 115; //Ground
			g = min(max(max(-nz,0)*1.4 - csamp[0]/32.0 + noise3d(x*(1.0/64.0),y*(1.0/64.0),.3,15)*.3,0),1);
			gr += (72-gr)*g; gg += (80-gg)*g; gb += (32-gb)*g; //Grass
			g2 = (1-fabs(g-.5)*2)*.7;
			gr += (68-gr)*g2; gg += (78-gg)*g2; gb += (40-gb)*g2; //Grass2
			g2 = max(min((samp[0]-csamp[0])*1.5,1),0);
			g = 1-g2*.2;
			gr += (60*g-gr)*g2; gg += (100*g-gg)*g2; gb += (120*g-gb)*g2; //Water


			d = .3;
			amb[k].r = (unsigned char)min(max(gr*d,0),255);
			amb[k].g = (unsigned char)min(max(gg*d,0),255);
			amb[k].b = (unsigned char)min(max(gb*d,0),255);
			maxa = max(max(amb[k].r,amb[k].g),amb[k].b);

            //lighting
			d = (nx*.5 + ny*.25 - nz)/sqrt(.5*.5 + .25*.25 + 1.0*1.0); d *= 1.2;
			// buf[k].a = (unsigned char)(175.0-samp[0]*((double)VSID/256.0));
			buf[k].a = (unsigned char)(63-samp[0]);
			buf[k].r = (unsigned char)min(max(gr*d,0),255-maxa);
			buf[k].g = (unsigned char)min(max(gg*d,0),255-maxa);
			buf[k].b = (unsigned char)min(max(gb*d,0),255-maxa);
		}
	}

	for(y=0,k=0;y<VSID;y++) {
		for(x=0;x<VSID;x++,k++) {
			buf[k].r += amb[k].r;
			buf[k].g += amb[k].g;
			buf[k].b += amb[k].b;
		}
	}
    
    int height, z, lowest_z;
    for (y = 0, k = 0; y < VSID; y++) {
    for (x = 0; x < VSID; x++, k++) {
        height = buf[k].a;
        for (z = 63; z > height; z--) {
			map_vxl_setgeom(x,y,z,1,map);
        }
		map_vxl_setgeom(x,y,z,1,map);
        lowest_z = get_lowest_height(x, y) + 1;
        for (; z < lowest_z; z++) {
			map_vxl_setcolor(x,y,z,((int*)&buf[k])[0], map);
        }
    }}

}