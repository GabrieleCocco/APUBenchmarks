#include <CL/cl.hpp>
#include <string.h>
#include <stdlib.h>

typedef struct CLMemorySetting {
	char* name;
	cl_mem_flags flags;
	unsigned int mapping;
} CLMemorySetting;

void clCreateMemorySetting(
	char* name, 
	cl_mem_flags flags,
	unsigned int mapping,
	CLMemorySetting* setting);

void clFreeMemorySetting(
	CLMemorySetting* setting);