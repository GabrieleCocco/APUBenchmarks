#define MAX_DEV_COUNT 4

#include "general.h"
#include <CLEnvironment.h>

typedef struct SaxpyProfilingData {
	CLDeviceEnvironment environments[MAX_DEV_COUNT];
	unsigned int kernel_index[MAX_DEV_COUNT];

	unsigned int global_sizes[MAX_DEV_COUNT];
	unsigned int local_sizes[MAX_DEV_COUNT];
	unsigned int data_sizes[MAX_DEV_COUNT];

	unsigned int device_count;	
	unsigned int type_size; 

	bool verify_output;
	float* reference;
	float a;

	cl_mem_flags src_flags[MAX_DEV_COUNT];
	cl_mem_flags dst_flags[MAX_DEV_COUNT]; 
	unsigned int map_src[MAX_DEV_COUNT];
	unsigned int map_dst[MAX_DEV_COUNT];

} SaxpyProfilingData;

typedef struct SaxpyDeviceData {
	SaxpyProfilingData* profiling_data;
	unsigned int offset;
	unsigned int index;
} SaxpyDeviceData;

int runHostComputation(void* data);
int runDeviceComputation(void* data);
int runHeteroSeparatedComputation(void* data);
DWORD WINAPI handleComputation(void* data);

void computeInput(float* x, float* y, unsigned int size);
void computeOutput(float* x, float* y, float* output, float a, unsigned int size);