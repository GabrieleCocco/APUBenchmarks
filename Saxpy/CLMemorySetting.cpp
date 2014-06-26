#include "CLMemorySetting.h"

void clCreateMemorySetting(
	char* name, 
	cl_mem_flags flags,
	unsigned int mapping,
	CLMemorySetting* setting) {
	
	setting->name = (char*)malloc(strlen(name) + 1);
	memset(setting->name, 0, strlen(name) + 1);
	strncpy(setting->name, name, strlen(name));

	setting->flags = flags;
	setting->mapping = mapping;
}

void clFreeMemorySetting(
	CLMemorySetting* setting) {
		free(setting->name);
}