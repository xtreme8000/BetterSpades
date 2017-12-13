extern struct RENDER_OPTIONS {
    char name[17];
	boolean opengl14;
	boolean color_correction;
	boolean shadow_entities;
	boolean ambient_occlusion;
	float render_distance;
	int window_width;
	int window_height;
	unsigned char multisamples;
	boolean player_arms;
	boolean fullscreen;
    boolean greedy_meshing;
    boolean vsync;
} settings;

void config_reload(void);
