#ifndef SAXPY_COMPUTATIONS_H
#define SAXPY_COMPUTATIONS_H

#define MAX_DEV_COUNT 4

#include <CLEnvironment.h>
#include <CLProfiling.h>

typedef struct SaxpyProfilingData {
	CLDeviceEnvironment* environments;
	unsigned int* kernel_index;

	unsigned int* global_sizes;
	unsigned int* local_sizes;
	unsigned int* data_sizes;

	unsigned int device_count;	
	unsigned int type_size; 

	bool verify_output;
	float* reference;
	float a;

	cl_mem_flags src_flags;
	cl_mem_flags dst_flags; 
	int map_src;
	int map_dst;
} SaxpyProfilingData;

typedef struct SaxpyDeviceData {
	SaxpyProfilingData* profiling_data;
	unsigned int offset;
	unsigned int index;
} SaxpyDeviceData;

CLProfilingResult runHostComputation(void* data);
CLProfilingResult runDeviceComputation(void* data);
CLProfilingResult runHeteroSeparatedComputation(void* data);
DWORD WINAPI handleComputation(void* data);

void computeInput(float* x, float* y, unsigned int size);
void computeOutput(float* x, float* y, float* output, float a, unsigned int size);
#endif