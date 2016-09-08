unsigned char* file_load(const char* name) {
	FILE *f;
	f = fopen(name,"rb");
	fseek(f,0,SEEK_END);
	int size = ftell(f);
	unsigned char* data = malloc(size);
	fseek(f,0,SEEK_SET);
	fread(data,size,1,f);
	fclose(f);
	return data;
}

unsigned int buffer_read32(unsigned char* buffer, int index) {
	return (buffer[index+3]<<24) | (buffer[index+2]<<16) | (buffer[index+1]<<8) | buffer[index];
}

unsigned short buffer_read16(unsigned char* buffer, int index) {
	return (buffer[index+1]<<8) | buffer[index];
}

unsigned char buffer_read8(unsigned char* buffer, int index) {
	return buffer[index];
}