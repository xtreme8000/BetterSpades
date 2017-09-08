#define GRENADES_MAX 64*3
/* if protocol is ever extended to 64 players,
each can throw all their grenades all at once */

struct Grenade {
    unsigned char active;
    unsigned char owner;
    float fuse_length;
    long created;
    struct Position pos;
    struct Velocity velocity;
} grenades[GRENADES_MAX];
