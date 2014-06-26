#include <CL/cl.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

/* Struct representing basic device info */
typedef struct CLDeviceInfo {
	cl_device_id id;
	char* name;
	cl_device_type type;
} CLDeviceInfo;

/* Struct representing the execution environment for one device */
typedef struct CLDeviceEnvironment {
	cl_context context;
	cl_platform_id platform;
	CLDeviceInfo info;
	cl_command_queue queue;
	cl_program program;
	cl_kernel* kernels;
	unsigned int kernels_count;
} CLDeviceEnvironment;

/* CL environment functions */
void clCreateDeviceEnvironment(
	CLDeviceInfo* devices,
	unsigned int device_count,
	const char* kernel_path,
	const char** kernel_functions,
	unsigned int kernel_functions_count,
	const char* build_options,
	unsigned int enable_gpu_profiling,
	unsigned int shared,
	CLDeviceEnvironment* output);

void clFreeDeviceEnvironments(
	CLDeviceEnvironment* environments,
	unsigned int device_count,
	unsigned int shared);

void clListDevices(
	unsigned int* count,
	CLDeviceInfo** output);

void clPrintDeviceList(
	CLDeviceInfo* devices,
	unsigned int count,
	const char* before_item);

void clDeviceTypeToString(
	cl_device_type type, 
	char** output);

void clPrintDeviceInfo(
	cl_device_id device);

void clFreeDeviceInfo(
	CLDeviceInfo* device);

void clErrorToString(
	cl_int error,
	char* output);

void clMemFlagsToString(
	cl_mem_flags flags,
	char* output);

/* Private functions */
void clLoadFile(
	const char* file,
	unsigned int* size,
	const char* * output);
/* End */

