#include "CLGeneral.h"
#include <stdlib.h>
#include <WinBase.h>

typedef CLUTILLIB_API struct Timer {
	LARGE_INTEGER frequency;
	LARGE_INTEGER start_time;
	void start() {
		QueryPerformanceFrequency(&frequency);	
		QueryPerformanceCounter(&start_time);
	}
	double get() {
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		double elapsedTime = (end.QuadPart - start_time.QuadPart) * 1000.0 / frequency.QuadPart;
		return elapsedTime;
	}
} Timer;

typedef CLUTILLIB_API enum CLProfilingResultOperation { 
	CL_PROFILING_RESULT_AVERAGE, 
	CL_PROFILING_RESULT_MAX, 
	CL_PROFILING_RESULT_MIN,
	CL_PROFILING_RESULT_SUM
} CLProfilingResultOperation;

typedef CLUTILLIB_API struct CLProfilingResult {
	bool success;
	double alloc_time;
	double init_time;
	double exec_time;
	double read_time;
} CLProfilingResult;

typedef CLProfilingResult (*CLProfiledComputation) (void* data);

CLUTILLIB_API long clSystemTimeToMillis(SYSTEMTIME time);
CLUTILLIB_API CLProfilingResult clProfileComputation(
	CLProfiledComputation computation,
	void* data,
	unsigned int samples,
	unsigned int profiling_duration,
	CLProfilingResultOperation operation,
	SYSTEMTIME* start,
	SYSTEMTIME* end,
	unsigned int* final_samples);
