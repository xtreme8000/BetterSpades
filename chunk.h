unsigned char chunk_geometry_rebuild = 1;
int chunk_geometry_rebuild_state = 0;

int chunk_display_lists[CHUNKS_PER_DIM*CHUNKS_PER_DIM];
int chunk_max_height[CHUNKS_PER_DIM*CHUNKS_PER_DIM];
int chunk_last_update[CHUNKS_PER_DIM*CHUNKS_PER_DIM];

int chunk_geometry_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM];
int chunk_geometry_changed_lenght = 0;

int chunk_lighting_changed[CHUNKS_PER_DIM*CHUNKS_PER_DIM];
int chunk_lighting_changed_lenght = 0;

boolean chunk_render_mode = 0;

void chunk_block_update(int x, int y, int z);
void chunk_update_all();
int chunk_generate(int displaylist, int chunk_x, int chunk_y);
void chunk_render(int x, int y);
void chunk_rebuild_all();
void chunk_set_render_mode(boolean r);
float chunk_draw_visible();