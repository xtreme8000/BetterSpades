struct kv6_t {
	unsigned short xsiz, ysiz, zsiz;
	float xpiv, ypiv, zpiv;
	unsigned char has_display_list, colorize;
	int display_list;
	unsigned int* color;
	float scale;
	unsigned char* colors_final;
	float* vertices_final;
	char* normals_final;
	int size;
	float red, green, blue;
};

extern struct kv6_t model_playerhead;
extern struct kv6_t model_playertorso;
extern struct kv6_t model_playertorsoc;
extern struct kv6_t model_playerarms;
extern struct kv6_t model_playerleg;
extern struct kv6_t model_playerlegc;
extern struct kv6_t model_intel;
extern struct kv6_t model_tent;

extern struct kv6_t model_semi;
extern struct kv6_t model_smg;
extern struct kv6_t model_shotgun;
extern struct kv6_t model_spade;
extern struct kv6_t model_block;
extern struct kv6_t model_grenade;

void kv6_render(struct kv6_t* kv6, unsigned char team);
struct kv6_t kv6_load(unsigned char* bytes, float scale);
void kv6_init(void);
void mul_matrix_vector(float* out, double* m, float* v);

extern float kv6_normals[256][3];
