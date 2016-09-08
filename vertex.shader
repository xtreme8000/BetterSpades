#version 110

uniform float point_size;
uniform float near_plane_height;

varying vec4 vertex_world_space;

void main(void) {
	vertex_world_space = gl_ModelViewMatrix*gl_Vertex;
	gl_Position = gl_ProjectionMatrix*vertex_world_space;
	gl_PointSize = (near_plane_height*point_size*2.5)/gl_Position.w;
	gl_FrontColor = gl_Color;
}