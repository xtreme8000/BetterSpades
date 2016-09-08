#version 110

uniform float camera_x;
uniform float camera_z;
uniform float fog_distance;
uniform float max_size_x;
uniform float max_size_z;
uniform vec4 fog_color;
uniform bool setting_color_correction;
uniform bool draw_ui;
uniform sampler3D Texture0;

varying vec4 vertex_world_space;

void main(void) {
	float distance = sqrt((vertex_world_space.x-camera_x)*(vertex_world_space.x-camera_x)+(vertex_world_space.z-camera_z)*(vertex_world_space.z-camera_z));
	float w = 1.0;
	if(distance>fog_distance*0.375) {
		w = 1.0-((distance-fog_distance*0.375)/(fog_distance*0.625));
	}
	w = max(min(w,1.0),0.0);
	gl_FragColor = gl_Color;
	if(!draw_ui) {
		if(vertex_world_space.x<0.0 || vertex_world_space.z<0.0 || vertex_world_space.x>max_size_x || vertex_world_space.z>max_size_z) {
			float l = (gl_FragColor.r+gl_FragColor.g+gl_FragColor.b)/3.0;
			gl_FragColor = vec4(l,l,l,1.0);
		}
	}
	if(setting_color_correction) {
		gl_FragColor = texture3D(Texture0, gl_FragColor.rgb);
	}
	if(!draw_ui) {
		gl_FragColor = gl_FragColor*w+fog_color*(1.0-w);
	}
}