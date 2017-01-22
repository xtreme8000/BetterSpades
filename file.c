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

//I now this can be cast to an unsigned int pointer more easily but this is endianess independent
unsigned int buffer_read32(unsigned char* buffer, int index) {
	return (buffer[index+3]<<24) | (buffer[index+2]<<16) | (buffer[index+1]<<8) | buffer[index];
}

//Same goes for this function
unsigned short buffer_read16(unsigned char* buffer, int index) {
	return (buffer[index+1]<<8) | buffer[index];
}

//Just to preserve function use
unsigned char buffer_read8(unsigned char* buffer, int index) {
	return buffer[index];
}
