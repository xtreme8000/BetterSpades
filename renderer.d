import derelict.sdl2.sdl;
import core.stdc.stdio : cstdio_fread=fread;
import std.algorithm;
import std.stdio;
import std.math;
import protocol;
import gfx;
import world;
import misc;
import vector;
import ui;
import core.stdc.stdlib : malloc, free;

SDL_GLContext gl_context;
uint Renderer_WindowFlags=SDL_WINDOW_OPENGL;

extern(C) void ogl_reshape(int width, int height);
extern(C) void ogl_init();
extern(C) void ogl_display();
extern(C) void ogl_camera_setup(float a, float b, float x, float y, float z);
extern(C) void ogl_map_vxl_load(char* data, int x, int y, int z);
extern(C) void ogl_chunk_rebuild_all();
extern(C) void ogl_map_set(int x, int y, int z, ulong color);
extern(C) ulong ogl_map_get(int x, int y, int z);
extern(C) void ogl_particle_create(uint color, float x, float y, float z, float velocity, float velocity_y, int amount, float min_size, float max_size);
extern(C) void ogl_overlay_setup();
extern(C) void ogl_overlay_rect(void* texture, int texture_width, int texture_height, ubyte red, ubyte green, ubyte blue, ubyte alpha, int x, int y, int w, int h);
extern(C) void ogl_overlay_rect_sub(void* texture, int texture_width, int texture_height, ubyte red, ubyte green, ubyte blue, ubyte alpha, int x, int y, int w, int h, int src_x, int src_y, int src_w, int src_h);
extern(C) void ogl_overlay_finish();
extern(C) int ogl_overlay_bind_fullness();
extern(C) void ogl_display_min();
extern(C) void ogl_render_sprite(char* filename, float x, float y, float z, int xsiz, int ysiz, int zsiz, float dx, float dy, float dz, float rx, float ry, float rz);
extern(C) void ogl_render_3d_box(float x, float y, float z, float size, uint color);
extern(C) void ogl_render_hole(float x, float y, float z, float size, uint color, ubyte side);
extern(C) int ogl_kv6_bind_fullness();
extern(C) void ogl_set_wireframe(float x, float y, float z);

extern(C) char* ogl_info(int i);
extern(C) int ogl_deprecation_state();

alias RendererTexture_t=void*;

uint screen_w = 0, screen_h = 0;
immutable float Renderer_SmokeRenderSpeed=1.0;
bool ogl_chunks_invalid = false;

void Renderer_Init(){
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,4);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,16);
}

void Renderer_SetUp(uint screen_xsize, uint screen_ysize) {
	if(screen_w==0 && screen_h==0) {
		gl_context = SDL_GL_CreateContext(scrn_window);
		ogl_init();
		printf("%s\n%s\n%s\n",ogl_info(0),ogl_info(1),ogl_info(2));
	}
	screen_w = screen_xsize;
	screen_h = screen_ysize;
	ogl_reshape(screen_xsize,screen_ysize);
}

void*[] allocated_textures;

void Renderer_UnInit(){
	foreach(tex; allocated_textures)
		free(tex);
}

RendererTexture_t Renderer_TextureFromSurface(SDL_Surface *srfc){
	void *mempos=malloc(srfc.w*srfc.h*4);
	if(!mempos){
		writeflnlog("SHTF: MALLOC FAILED\n");
	}
	(cast(ubyte*)mempos)[0..srfc.w*srfc.h*4]=(cast(ubyte*)srfc.pixels)[0..srfc.w*srfc.h*4];
	RendererTexture_t ret=mempos;
	allocated_textures~=ret;
	return cast(RendererTexture_t)ret;
}

void Renderer_UploadToTexture(SDL_Surface *srfc, RendererTexture_t tex){
	(cast(uint*)tex)[0..srfc.w*srfc.h]=(cast(uint*)srfc.pixels)[0..srfc.w*srfc.h];
}

void Renderer_DestroyTexture(RendererTexture_t tex){
	if(allocated_textures.canFind(tex))
		allocated_textures=allocated_textures.remove(allocated_textures.countUntil(tex));
	free(tex);
}

RendererTexture_t Renderer_NewTexture(uint xsize, uint ysize, bool streaming_texture=false){
	return null;
}

void Renderer_Blit2D(RendererTexture_t tex, uint[2]* size, SDL_Rect *dstr, ubyte alpha=255, ubyte[3] *ColorMod=null, SDL_Rect *srcr=null){
	ubyte[3] cmod;
	if(ColorMod){
		cmod=*ColorMod;
	}
	else{
		cmod=[255, 255, 255];
	}
	if(!srcr){
		ogl_overlay_rect(tex,(*size)[0],(*size)[1], cmod[0], cmod[1], cmod[2], alpha, dstr.x, dstr.y, dstr.w, dstr.h);
	}
	else{
		ogl_overlay_rect_sub(tex,(*size)[0],(*size)[1], cmod[0], cmod[1], cmod[2], alpha, dstr.x, dstr.y, dstr.w, dstr.h,
		srcr.x, srcr.y, srcr.w, srcr.h);
	}
}

int Rendered_3D=-1;

void Renderer_StartRendering(bool render_3D){
	Rendered_3D=render_3D;
	if(render_3D)
		ogl_display();
	else
		ogl_display_min();
}

void Renderer_Start2D(){
	ogl_overlay_setup();
}

void Renderer_ShowInfo(){
	int state = ogl_deprecation_state();
	if(state>0) {
		Render_Text_Line(0,cast(int)(screen_h-FontHeight/16*2.5F),0xFFFFFF,"GPU deprecated!",font_texture,FontWidth,FontHeight,LetterPadding,2.5F,2.5F);
		if(state&1) {
			Render_Text_Line(0,cast(int)(screen_h-FontHeight/16*2.5F*2.0F),0xFFFFFF,"power of 2 tex limit",font_texture,FontWidth,FontHeight,LetterPadding,2.5F,2.5F);
		}
		if(state&2) {
			Render_Text_Line(0,cast(int)(screen_h-FontHeight/16*2.5F*3.0F),0xFFFFFF,"ogl1.4 only",font_texture,FontWidth,FontHeight,LetterPadding,2.5F,2.5F);
		}
	}
}

void Renderer_Finish2D(){
	ogl_overlay_finish();
}

void Renderer_FinishRendering(){
	SDL_GL_SwapWindow(scrn_window);
}

void Renderer_LoadMap(ubyte[] map){
	ogl_map_vxl_load(cast(char*)map.ptr,MapXSize,MapYSize,MapZSize);
}

void Renderer_SetCamera(float xrotation, float yrotation, float tilt, float xfov, float yfov, float xpos, float ypos, float zpos){
	ogl_camera_setup((-xrotation+90.0F)/180.0F*3.14159F, (yrotation+90.0F)/180.0F*3.14159F, xpos, 64.0-ypos, zpos);
}

void Renderer_FillRect(SDL_Rect *rct, ubyte[4] *color) {

}

void Renderer_FillRect(SDL_Rect *rct, uint color) {

}

void Renderer_DrawVoxels(){}

//PLS ADD: (returns whether object is visible, scrx/scry are screen coords of the centre, dist is distance
bool Project2D(float xpos, float ypos, float zpos, out int scrx, out int scry, float *dist){
	return false;
}

int[2] Project2D(float xpos, float ypos, float zpos, float *dist){
	return [-10000, -10000];
}

void Renderer_SetBlur(float amount) {

}

uint Voxel_GetHighestY(uint x, uint y, uint z){
	for(y=0;y<MapYSize; y++){
		if(Voxel_IsSolid(x, y, z)){
			return y;
		}
	}
	return 0;
}

bool Voxel_IsSolid(Tx, Ty, Tz)(Tx x, Ty y, Tz z) {
	return ogl_map_get(cast(uint)x,cast(uint)(64-y-1),cast(uint)z)!=0xFFFFFFFF;
}

bool Voxel_IsSolid(Vector3_t pos){
	return Voxel_IsSolid(pos.x, pos.y, pos.z);
}

void Voxel_SetColor(uint x, uint y, uint z, uint col){
	ogl_map_set(x,64-y-1,z,((col>>16)&255) | (col&0xFF00) | ((col&255)<<16));
}

void Voxel_SetShade(uint x, uint y, uint z, ubyte shade){

}

ubyte Voxel_GetShade(uint x, uint y, uint z) {
	return 0;
}

void _Register_Lighting_BBox(int xpos, int ypos, int zpos) {

}

uint Voxel_GetColor(uint x, uint y, uint z){
	uint col=ogl_map_get(x,64-y-1,z)&0xFFFFFF;
	return ((col>>16)&255) | (col&0xFF00) | ((col&255)<<16);
}

void Voxel_Remove(uint x, uint y, uint z){
	uint col = ogl_map_get(x,64-y-1,z)&0xFFFFFF;
	ogl_map_set(x,64-y-1,z,0xFFFFFFFF);
}

void Renderer_DrawSprite(Sprite_t *spr){
	ogl_render_sprite(cast(char*)spr.model.filename.ptr,spr.xpos,64.0F-spr.ypos,spr.zpos,spr.model.xsize,spr.model.ysize,spr.model.zsize,spr.xdensity,spr.ydensity,spr.zdensity,spr.rhe,-spr.rti+90.0F,spr.rst);
}

void Renderer_DrawWireframe(Sprite_t *spr) {
	ogl_set_wireframe(spr.xpos,64-spr.ypos-1,spr.zpos);
}

void Renderer_SetFog(uint fogcolor, uint fogrange){
}

alias RendererParticleSize_t=float;

extern(C) struct ModelVoxel_t {
	uint color;
	ushort ypos;
	char visiblefaces, normalindex;
}

extern(C){
	struct Model_t{
		int xsize, ysize, zsize;
		float xpivot, ypivot, zpivot;
		Model_t *lowermip;
		ModelVoxel_t[] voxels;
		uint[] offsets;
		ushort[] column_lengths;
		string filename;
		Model_t *copy(){
			Model_t *newmodel=new Model_t;
			newmodel.xsize=xsize; newmodel.ysize=ysize; newmodel.zsize=zsize;
			newmodel.xpivot=xpivot; newmodel.ypivot=ypivot; newmodel.zpivot=zpivot;
			newmodel.lowermip=lowermip;
			newmodel.voxels.length=voxels.length; newmodel.voxels[]=voxels[];
			newmodel.offsets.length=offsets.length; newmodel.offsets[]=offsets[];
			newmodel.column_lengths.length=column_lengths.length; newmodel.column_lengths[]=column_lengths[];
			return newmodel;
		}
	}

	struct Sprite_t{
		float rhe, rti, rst;
		union{
			struct{
				float xpos, ypos, zpos;
			}
			Vector3_t pos;
		}
		float xdensity, ydensity, zdensity;
		uint color_mod, replace_black;
		ubyte brightness;
		ubyte check_visibility;
		Model_t *model;
	}
}

int freadptr(void *buf, uint bytes, File f){
	if(!buf){
		writeflnlog("freadptr called with void buffer");
		return 0;
	}
	return cast(int)cstdio_fread(buf, bytes, 1u, f.getFP());
}

Model_t *Load_KV6(string fname){
	File f=File(fname, "rb");
	if(!f.isOpen()){
		writeflnerr("Couldn't open %s", fname);
		return null;
	}
	string fileid;
	fileid.length=4;
	freadptr(cast(void*)fileid.ptr, 4, f);
	if(fileid!="Kvxl"){
		writeflnerr("Model file %s is not a valid KV6 file (wrong header)", fname);
		return null;
	}
	Model_t *model=new Model_t;
	model.filename=fname;
	freadptr(&model.xsize, 4, f); freadptr(&model.zsize, 4, f); freadptr(&model.ysize, 4, f);
	if(model.xsize<0 || model.ysize<0 || model.zsize<0){
		writeflnerr("Model file %s has invalid size (%d|%d|%d)", fname, model.xsize, model.ysize, model.zsize);
		return null;
	}
	freadptr(&model.xpivot, 4, f); freadptr(&model.zpivot, 4, f); freadptr(&model.ypivot, 4, f);
	int voxelcount;
	freadptr(&voxelcount, voxelcount.sizeof, f);
	if(voxelcount<0){
		writeflnerr("Model file %s has invalid voxel count (%d)", fname, voxelcount);
		return null;
	}
	model.voxels=new ModelVoxel_t[](voxelcount);
	for(uint i=0; i<voxelcount; i++){
		freadptr(&model.voxels[i], model.voxels[i].sizeof, f);
	}
	auto xlength=new uint[](model.xsize);
	for(uint x=0; x<model.xsize; x++)
		freadptr(&xlength[x], 4, f);
	auto ylength=new ushort[][](model.xsize, model.zsize);
	for(uint x=0; x<model.xsize; x++)
		for(uint z=0; z<model.zsize; z++)
			freadptr(&ylength[x][z], 2, f);
	model.offsets.length=model.xsize*model.zsize;
	model.column_lengths.length=model.offsets.length;
	typeof(model.offsets[0]) voxel_xindex=0;
	for(uint x=0; x<model.xsize; x++){
		auto voxel_zindex=voxel_xindex;
		for(uint z=0; z<model.zsize; z++){
			model.offsets[x+z*model.xsize]=voxel_zindex;
			model.column_lengths[x+z*model.xsize]=ylength[x][z];
			voxel_zindex+=ylength[x][z];
		}
		voxel_xindex+=xlength[x];
	}
	string palette;
	palette.length=4;
	freadptr(cast(void*)palette.ptr, 4, f);
	if(!f.eof()){
		if(palette=="SPal"){
			writeflnlog("Note: File %s contains a useless suggested palette block (SLAB6)", fname);
		}
		else{
			writeflnlog("Warning: File %s contains invalid data after its ending (corrupted file?)", fname);
			writeflnlog("KV6 size: (%d|%d|%d), pivot: (%d|%d|%d), amount of voxels: %d", model.xsize, model.ysize, model.zsize, 
			model.xpivot, model.ypivot, model.zpivot, voxelcount);
		}
	}
	f.close();
	return model;
}

void Renderer_Draw3DParticle(float x, float y, float z, RendererParticleSize_t sx, RendererParticleSize_t sy, RendererParticleSize_t sz, uint col) {
	ogl_render_3d_box(x,64.0F-y+0.05F,z,(sx+sy+sz)/3.0F,col);
}

/*void Renderer_Draw3DBlockDamage(float x, float y, float z, uint col, ubyte side) {
	ogl_render_hole(x,64.0F-y,z,0.1F,col,side);
}*/

void Renderer_Draw3DParticle(Vector3_t *pos, RendererParticleSize_t sx, RendererParticleSize_t sy, RendererParticleSize_t sz, uint col){
	return Renderer_Draw3DParticle(pos.x, pos.y, pos.z, sx, sy, sz, col);
}

RendererParticleSize_t[3] Renderer_GetParticleSize(float x, float y, float z) {
	RendererParticleSize_t[3] a;
	a[0] = x;
	a[1] = y;
	a[2] = z;
	return a;
}

void Renderer_DrawSmokeCircle(float x, float y, float z, int radius, uint color, uint alpha, float dist){

}

struct FlashVoxelLink_t{
	float brightness;
	Flash_t *flash;
}

struct FlashVoxel_t{
	uint x, y, z;
	ubyte original_shade;
	ubyte *shade;
	FlashVoxelLink_t[] flashes;
}

struct Flash_t{
	Vector3_t centre;
	uint[] voxels;
	float radius;
	float timer;
	float decay;
}

Flash_t*[] Flashes;
FlashVoxel_t[uint] FlashVoxels;

void Renderer_AddFlash(Vector3_t pos, float radius, float brightness){
	
}

void Renderer_UpdateFlashes(alias UpdateGfx=true)(float update_speed){
	
}

auto Renderer_DrawRoundZoomedIn(Vector3_t* scope_pos, Vector3_t* scope_rot, MenuElement_t *scope_picture, float xzoom, float yzoom) {
	struct return_type{
		SDL_Rect dstrect;
		SDL_Rect srcrect;
		SDL_Texture *scope_texture;
		uint scope_texture_width, scope_texture_height;
	}
	return_type *ret=null;
	return ret;
}

void Renderer_SetBrightness(float brightness){
	
}

void Renderer_SetQuality(float quality){
	
}

void Renderer_SetBlockFaceShading(Vector3_t shading) {
	
}
