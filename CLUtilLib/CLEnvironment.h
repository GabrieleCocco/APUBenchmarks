#include "CLGeneral.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

/* Struct representing basic device info */
typedef CLUTILLIB_API struct CLDeviceInfo {
	cl_device_id id;
	char* name;
	cl_device_type type;
} CLDeviceInfo;

/* Struct representing the execution environment for one device */
typedef CLUTILLIB_API struct CLDeviceEnvironment {
	cl_context context;
	cl_platform_id platform;
	CLDeviceInfo info;
	cl_command_queue queue;
	cl_program program;
	cl_kernel* kernels;
	unsigned int kernels_count;
} CLDeviceEnvironment;

/* CL environment functions */
CLUTILLIB_API void clCreateDeviceEnvironment(
	CLDeviceInfo* devices,
	unsigned int device_count,
	const char* kernel_path,
	const char** kernel_functions,
	unsigned int kernel_functions_count,
	const char* build_options,
	unsigned int enable_gpu_profiling,
	unsigned int shared,
	CLDeviceEnvironment* output);

CLUTILLIB_API void clFreeDeviceEnvironments(
	CLDeviceEnvironment* environments,
	unsigned int device_count,
	unsigned int shared);

CLUTILLIB_API void clListDevices(
	unsigned int* count,
	CLDeviceInfo** output);

CLUTILLIB_API void clPrintDeviceList(
	CLDeviceInfo* devices,
	unsigned int count,
	const char* before_item);

CLUTILLIB_API void clDeviceTypeToString(
	cl_device_type type, 
	char** output);

CLUTILLIB_API void clPrintDeviceInfo(
	cl_device_id device);

CLUTILLIB_API void clFreeDeviceInfo(
	CLDeviceInfo* device);

CLUTILLIB_API void clErrorToString(
	cl_int error,
	char* output);

CLUTILLIB_API void clMemFlagsToString(
	cl_mem_flags flags,
	char* output);

/* Private functions */
void clLoadFile(
	const char* file,
	unsigned int* size,
	const char* * output);
/* End */

