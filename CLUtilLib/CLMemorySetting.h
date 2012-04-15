#include "CLGeneral.h"
#include <string.h>
#include <stdlib.h>

typedef CLUTILLIB_API struct CLMemorySetting {
	char* name;
	cl_mem_flags flags;
	unsigned int mapping;
} CLMemorySetting;

void CLUTILLIB_API clCreateMemorySetting(
	char* name, 
	cl_mem_flags flags,
	unsigned int mapping,
	CLMemorySetting* setting);

void CLUTILLIB_API clFreeMemorySetting(
	CLMemorySetting* setting);