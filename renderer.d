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
extern(C) void ogl_render_sprite(ubyte* filename, float x, float y, float z, int xsiz, int ysiz, int zsiz, float dx, float dy, float dz, float rx, float ry, float rz);
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
	/*SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,4);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,16);*/
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

uint Voxel_GetHighestY(Tx, Ty, Tz)(Tx x, Ty y, Tz z){
	for(uint y2=cast(uint)y;y2<MapYSize; y2++){
		if(Voxel_IsSolid(cast(uint)x, cast(uint)y2, cast(uint)z)){
			return y2;
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

void Renderer_DrawSprite(SpriteRenderData_t *sprrend, Vector3_t pos, Vector3_t rotation){
	Sprite_t spr;
	spr.model=sprrend.model;
	spr.pos=pos; spr.rot=rotation; spr.density=sprrend.size/Vector3_t(spr.model.size);
	spr.color_mod=sprrend.color_mod; spr.replace_black=sprrend.replace_black;
	spr.brightness=sprrend.brightness; spr.check_visibility=sprrend.check_visibility;
	return Renderer_DrawSprite(&spr);
}

struct _Renderer_ModelAttachment_t{
	ubyte[] raw_model;
}
alias Renderer_ModelAttachment_t=_Renderer_ModelAttachment_t;

void Renderer_DrawSprite(Sprite_t *spr){
	if(!spr.model.renderer_attachment.raw_model.length){
		spr.model.renderer_attachment.raw_model=spr.model.to!"KV6"();
	}
	import std.conv, std.string;
	ogl_render_sprite(spr.model.renderer_attachment.raw_model.ptr,spr.xpos,64.0F-spr.ypos,spr.zpos,spr.model.xsize,spr.model.ysize,spr.model.zsize,spr.xdensity,spr.ydensity,spr.zdensity,spr.rhe,-spr.rti+90.0F,spr.rst);
}

void Renderer_DrawWireframe(Sprite_t *spr) {
	ogl_set_wireframe(spr.xpos,64-spr.ypos-1,spr.zpos);
}

void Renderer_SetFog(uint fogcolor, uint fogrange){
}

alias RendererParticleSize_t=float;

import std.random;
void Renderer_Draw3DParticle(alias hole_side=false)(float x, float y, float z, RendererParticleSize_t sx, RendererParticleSize_t sy, RendererParticleSize_t sz, uint col) {
	static if(hole_side!=false && 0){
		ubyte hole_side=uniform!ubyte();
		if(x==cast(int)x)
			hole_side=0;
		else if(y==cast(int)y)
			hole_side=1;
		else if(z==cast(int)z)
			hole_side=2;
		ogl_render_hole(x,64.0F-y,z,0.1F,col,hole_side);
	}
	else{
		ogl_render_3d_box(x,64.0F-y+0.05F,z,(sx+sy+sz)/3.0F,col);
	}
}

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
