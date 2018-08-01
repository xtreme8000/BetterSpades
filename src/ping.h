struct ping_entry {
	ENetAddress addr;
	float time_start;
	char user_data[32];
	int trycount;
};

void ping_init();
void ping_deinit();
void ping_check(char* addr, int port, void* user_data);
void* ping_update(void* data);
void ping_start(void (*finished) (),  void (*result) (void*, float, void*));
void ping_lan();
