#include "CLProfiling.h"

CLUTILLIB_API long clSystemTimeToMillis(SYSTEMTIME time) {
	long millis = 0;
	millis += (time.wHour * 60 * 60 * 1000);
	millis += (time.wMinute * 60 * 1000);
	millis += (time.wSecond * 1000);
	millis += time.wMilliseconds;

	return millis;
}

CLUTILLIB_API CLProfilingResult clProfileComputation(
	CLProfiledComputation computation,
	void* data,
	unsigned int samples,
	unsigned int profiling_duration,
	CLProfilingResultOperation operation,
	SYSTEMTIME* start,
	SYSTEMTIME* end,
	unsigned int* final_samples) {
		
	CLProfilingResult result;

	if(profiling_duration > 0) {
		//Try to check sample duration
		GetSystemTime(start);
		result = computation(data);		
		if(!result.success)
			return result;
		GetSystemTime(end);
		long millis = clSystemTimeToMillis(*end) - clSystemTimeToMillis(*start);
		if(millis == 0) {
			//Try with 100 samples
			GetSystemTime(start);
			for(unsigned int i = 0; i < 100; i++) {
				result = computation(data);		
				if(!result.success)
					return result;
			}
			GetSystemTime(end);
			long millis = clSystemTimeToMillis(*end) - clSystemTimeToMillis(*start);
			if(millis > 0)
				*final_samples = (unsigned int)(((double)profiling_duration) / (double)millis) * 100;
			else
				*final_samples = 100000;
		}
		else
			*final_samples = (unsigned int)((double)profiling_duration) / (double)millis;
	}
	else
		*final_samples = samples;

	if(*final_samples == 0)
		*final_samples = 1;

	/* Allocate results vector */
	CLProfilingResult* results = (CLProfilingResult*)malloc(*final_samples * sizeof(CLProfilingResult));

	GetSystemTime(start);
	for(unsigned int i = 0; i < *final_samples; i++) {
		results[i] = computation(data);				
		if(!results[i].success) {
			free(results);
			return results[i];
		}
	}
	GetSystemTime(end);

	result.alloc_time = 0;
	result.exec_time = 0;
	result.init_time= 0;
	result.read_time = 0;
	result.success = 1;
	for(unsigned int i = 0; i < *final_samples; i++) {
		if(operation == CL_PROFILING_RESULT_AVERAGE) {
			result.alloc_time += results[i].alloc_time;
			result.exec_time += results[i].exec_time;
			result.init_time += results[i].init_time;
			result.read_time += results[i].read_time;
		}
		if(operation == CL_PROFILING_RESULT_SUM) {
			result.alloc_time += results[i].alloc_time;
			result.exec_time += results[i].exec_time;
			result.init_time += results[i].init_time;
			result.read_time += results[i].read_time;
		}
		if(operation == CL_PROFILING_RESULT_MAX) {
			result.alloc_time = results[i].alloc_time > result.alloc_time ? results[i].alloc_time : result.alloc_time;
			result.exec_time = results[i].exec_time > result.exec_time ? results[i].exec_time : result.exec_time;
			result.init_time = results[i].init_time > result.init_time ? results[i].init_time : result.init_time;
			result.read_time = results[i].read_time > result.read_time ? results[i].read_time : result.read_time;
		}
		if(operation == CL_PROFILING_RESULT_MIN) {
			result.alloc_time = results[i].alloc_time < result.alloc_time ? results[i].alloc_time : result.alloc_time;
			result.exec_time = results[i].exec_time < result.exec_time ? results[i].exec_time : result.exec_time;
			result.init_time = results[i].init_time < result.init_time ? results[i].init_time : result.init_time;
			result.read_time = results[i].read_time < result.read_time ? results[i].read_time : result.read_time;
		}
		result.success &= results[i].success;
	}
	
	if(operation == CL_PROFILING_RESULT_AVERAGE) {
		result.alloc_time = result.alloc_time / (double)(*final_samples); 
		result.exec_time = result.exec_time / (double)(*final_samples); 
		result.init_time = result.init_time / (double)(*final_samples); 
		result.read_time = result.read_time / (double)(*final_samples); 
	}
	free(results);
	return result;
}
