struct hud {
    void (*init) ();
    void (*render_3D) ();
    void (*render_2D) (float scalex, float scaley);
    void (*input_keyboard) (int key, int action, int mods);
    void (*input_mouselocation) (double x, double y);
    void (*input_mouseclick) (int button, int action, int mods);
    void (*input_mousescroll) (double yoffset);
    char render_world;
    char render_localplayer;
};

extern int screen_current;

extern struct hud hud_ingame;
extern struct hud hud_mapload;
extern struct hud hud_serverlist;

extern struct hud* hud_active;

void hud_change(struct hud* new);
void hud_init(GLFWwindow* window);
void hud_mousemode(int mode);
