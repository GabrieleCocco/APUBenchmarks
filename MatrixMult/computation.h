#include <CLEnvironment.h>
#include <CLProfiling.h>

typedef struct MatrixMultProfilingData {
	CLEnvironment* env;
	cl_kernel kernel;
	unsigned int size; 
	unsigned int threads; 
	unsigned int local_size; 
	unsigned int block_size;
	bool simulate_read;
	bool verify_output;
	unsigned int width_a;
	unsigned int height_a;
	unsigned int cpu_threads;
	cl_mem_flags src_flags;
	cl_mem_flags dst_flags; 
	int map_src;
	int map_dst;
} MatrixMultProfilingData;

CLProfilingResult runHostComputation(void* data);
CLProfilingResult runGpuComputation(void* data);
CLProfilingResult runOptimizedGpuComputation(void* data);
CLProfilingResult runHostGpuComputation(void* data);
CLProfilingResult runOptimizedHostGpuComputation(void* data);