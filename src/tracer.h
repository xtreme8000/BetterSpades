#define TRACER_MAX PLAYERS_MAX*5

struct Tracer {
    Ray r;
    unsigned char type;
    float created;
    unsigned char used;
};

extern struct Tracer* tracers;

void tracer_add(unsigned char type, char scoped, float x, float y, float z, float dx, float dy, float dz);
void tracer_update(float dt);
void tracer_render(void);
void tracer_init(void);
