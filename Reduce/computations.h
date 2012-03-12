#include <CLEnvironment.h>
#include <CLProfiling.h>
#include "general.h"

typedef struct ReduceProfilingData {
	CLDeviceEnvironment* environments;
	unsigned int* kernel_index;

	unsigned int* global_sizes;
	unsigned int* local_sizes;
	unsigned int* data_sizes;

	unsigned int device_count;	
	unsigned int type_size; 

	bool verify_output;
	int reference;
	
	cl_mem_flags src_flags;
	cl_mem_flags dst_flags; 
	int map_src;
	int map_dst;
} ReduceProfilingData;

typedef struct ReduceDeviceData {
	ReduceProfilingData* profiling_data;
	unsigned int offset;
	unsigned int index;
} ReduceDeviceData;

CLProfilingResult runHostComputation(void* data);
CLProfilingResult runDeviceComputation(void* data);
CLProfilingResult runHeteroSeparatedComputation(void* data);
DWORD WINAPI handleComputation(void* data);