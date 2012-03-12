#include <CL/cl.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>

/* Program option type (int string, float or bool) */
typedef enum CLProgramOptionType { 
	CL_PROGRAM_OPTION_INT, 
	CL_PROGRAM_OPTION_STRING, 
	CL_PROGRAM_OPTION_FLOAT, 
	CL_PROGRAM_OPTION_BOOL
} CLProgramOptionType;

/* Single or ultiple option values */
typedef enum CLProgramOptionVariant { 
	CL_PROGRAM_OPTION_SINGLE, 
	CL_PROGRAM_OPTION_MULTIPLE
} CLProgramOptionVariant;

/* Struct representing a program option */
typedef struct CLProgramOption {
	CLProgramOptionType type;
	CLProgramOptionVariant variant;
	char* name;
	char* keyword;
	void* value;
	unsigned int count;
} CLProgramOption;

/* Program option functions */
void clPrintProgramOptions(
	CLProgramOption* options[], 
	unsigned int num_options);

void clHandleProgramOptions(
	CLProgramOption* options[], 
	unsigned int num_options);

void clCreateDefaultProgramOptions(
	CLProgramOption* kernel_path, 
	const char* def_kernel_path,
	CLProgramOption* kernel_functions, 
	const char* def_kernel_functions, 
	unsigned int def_kernel_functions_count,
	CLProgramOption* kernel_options, 
	const char* def_kernel_options,
	CLProgramOption* kernel_profiling, 
	const char* def_kernel_profiling,
	CLProgramOption* local_size, 
	const char* def_local_size,
	CLProgramOption* log_file, 
	const char* def_log_file,
	CLProgramOption* append_to_file, 
	const char* def_append_to_file);

void clCreateProgramOption(
	char* name, 
	char* keyword, 
	CLProgramOptionType type,
	CLProgramOptionVariant variant,
	const char* value,
	unsigned int count,
	CLProgramOption* option);

void clFreeProgramOption(
	CLProgramOption* option);

const char* clProgramOptionAsString(
	CLProgramOption* option);

int clProgramOptionAsInt(
	CLProgramOption* option);

float clProgramOptionAsFloat(
	CLProgramOption* option);

bool clProgramOptionAsBool(
	CLProgramOption* option);

const char** clProgramOptionAsStringArray(
	CLProgramOption* option);

int* clProgramOptionAsIntArray(
	CLProgramOption* option);

float* clProgramOptionAsFloatArray(
	CLProgramOption* option);

bool* clProgramOptionAsBoolArray(
	CLProgramOption* option);
