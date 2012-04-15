#include <CLEnvironment.h>
#include <CLProfiling.h>
#include <CLSize.h>
#include <time.h>

#define MAX_DEV_COUNT 4

typedef struct MatrixMultProfilingData {
	CLDeviceEnvironment* environments;
	unsigned int* kernel_index;

	CLSize2D* global_sizes;
	CLSize1D* local_sizes;
	CLSize2D* a_data_sizes;
	CLSize1D* b_data_sizes;

	unsigned int device_count;	
	unsigned int type_size; 

	bool verify_output;
	float* reference;
	
	cl_mem_flags src_flags;
	cl_mem_flags dst_flags; 
	int map_src;
	int map_dst;
} MatrixMultProfilingData;

typedef struct MatrixMultDeviceData {
	MatrixMultProfilingData* profiling_data;
	unsigned int offset;
	unsigned int index;
} MatrixMultDeviceData;

CLProfilingResult runHostComputation(void* data);
CLProfilingResult runDeviceComputation(void* data);
CLProfilingResult runHeteroSeparatedComputation(void* data);
DWORD WINAPI handleComputation(void* data);

void computeInput(float* a, float* b, unsigned int width_a, unsigned int height_a, unsigned int width_b, unsigned int offset);
void computeOutput(float* a, float* b, float* c, unsigned int width_a, unsigned int height_a, unsigned int width_b);