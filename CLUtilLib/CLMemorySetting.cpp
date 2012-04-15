#include "CLMemorySetting.h"

CLUTILLIB_API void clCreateMemorySetting(
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

CLUTILLIB_API void clFreeMemorySetting(
	CLMemorySetting* setting) {
		free(setting->name);
}