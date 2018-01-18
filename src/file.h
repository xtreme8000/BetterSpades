char file_exists(const char* name);
unsigned char* file_load(const char* name);
float buffer_readf(unsigned char* buffer, int index);
unsigned int buffer_read32(unsigned char* buffer, int index);
unsigned short buffer_read16(unsigned char* buffer, int index);
unsigned char buffer_read8(unsigned char* buffer, int index);

#ifdef OS_WINDOWS
#define file_url(url) system("start "url)
#endif

#if defined(OS_LINUX) ||  defined(OS_APPLE)
#define file_url(url) system("open "url)
#endif
