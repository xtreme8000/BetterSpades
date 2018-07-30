struct ping_entry {
	ENetAddress addr;
	float time_start;
	void* user_data;
	int trycount;
	void (*callback) (struct ping_entry*, float time_delta, void* user_data);
};

void ping_init();
void ping_deinit();
void ping_check(char* addr, int port, void* user_data, void (*callback) (struct ping_entry*, float time_delta, void* user_data));
void* ping_update(void* data);
void ping_start(void (*callback) ());
