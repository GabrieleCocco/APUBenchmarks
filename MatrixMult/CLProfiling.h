#include "CLUtil.h"
#include <CL/cl.hpp>
#include <stdlib.h>
#include <WinBase.h>

typedef enum CLProfilingResultOperation { 
	CL_PROFILING_RESULT_AVERAGE, 
	CL_PROFILING_RESULT_MAX, 
	CL_PROFILING_RESULT_MIN,
	CL_PROFILING_RESULT_SUM
} CLProfilingResultOperation;

typedef struct CLProfilingResult {
	bool success;
	double alloc_time;
	double init_time;
	double exec_time;
	double read_time;
} CLProfilingResult;

typedef CLProfilingResult (*CLProfiledComputation) (void* data);

CLProfilingResult clProfileComputation(
	CLProfiledComputation computation,
	void* data,
	unsigned int samples,
	unsigned int profiling_duration,
	CLProfilingResultOperation operation,
	SYSTEMTIME* start,
	SYSTEMTIME* end,
	unsigned int* final_samples);
