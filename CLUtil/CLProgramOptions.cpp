#include "CLProgramOptions.h"

char** explode(std::string input, char delimiter, unsigned int* count) {
	if(input.find(delimiter) == std::string::npos) {
		*count = 1;
		char** pointer = (char**)malloc(1 * sizeof(char*));
		pointer[0] = (char*)malloc(input.length() + 1);
		strncpy(pointer[0], input.c_str(), input.length());
		pointer[0][input.length()] = 0;
		return pointer;
	}
	else {
		unsigned int start = 0;
		unsigned int end = 0;
		*count = 0;
		char** pointer = (char**)malloc(256 * sizeof(char*));
		for(unsigned int i = 0; i < input.length(); i++) {
			if(input[i] == delimiter) {
				pointer[*count] = (char*)malloc(end - start + 1);
				strncpy(pointer[*count], input.substr(start, end).c_str(), end - start);
				pointer[*count][end - start] = 0;
				(*count)++;
				start = end + 1;
			}
			end++;
		}
		pointer[*count] = (char*)malloc(end - start + 1);
		strncpy(pointer[*count], input.substr(start, end).c_str(), end - start);
		pointer[*count][end - start] = 0;
		(*count)++;

		return pointer;
	}
}

void parseProgramOptionValue(CLProgramOption* option, const char* value) {
	unsigned int value_count;
	char** values = explode(std::string(value), ',', &value_count);
	option->count = value_count;
	switch(option->type) {
		case CL_PROGRAM_OPTION_INT: {
			option->value = malloc(value_count * sizeof(int));
			for(unsigned int i = 0; i < value_count; i++) {
				((int*)option->value)[i] = atoi(values[i]);
				delete values[i];
			}
			free(values);
		} break;
		case CL_PROGRAM_OPTION_FLOAT: {
			option->value = malloc(value_count * sizeof(float));
			for(unsigned int i = 0; i < value_count; i++) {
				((float*)option->value)[i] = atof(values[i]);
				delete values[i];
			}
			free(values);
		} break;
		case CL_PROGRAM_OPTION_BOOL: {
			option->value = malloc(value_count * sizeof(bool));
			for(unsigned int i = 0; i < value_count; i++) {
				((bool*)option->value)[i] = (values[i][0]) == 'y'? true : false;
				delete values[i];
			}
			free(values);
		} break;
		default: {				
			option->value = malloc(value_count * sizeof(char*));
			for(unsigned int i = 0; i < value_count; i++) {
				((char**)option->value)[i] = (char*)malloc(strlen(values[i]) + 1);
				strncpy(((char**)option->value)[i], values[i], strlen(values[i]));
				((char**)option->value)[i][strlen(values[i])] = 0;
				free(values[i]);
			}
			free(values);	
		} break;
	}
}

void printProgramOptionValue(CLProgramOption* option, char** value) {
	if(option->value != NULL) {
		std::stringstream s(std::stringstream::out);

		switch(option->type) {
			case CL_PROGRAM_OPTION_INT: {
				for(unsigned int j = 0; j < option->count; j++) {
					if(j == 0)
						s << ((int*)(option->value))[j];
					else
						s << ", " << ((int*)(option->value))[j];
				}
			} break;
			case CL_PROGRAM_OPTION_FLOAT: {
				for(unsigned int j = 0; j < option->count; j++) {
					if(j == 0)
						s << ((float*)(option->value))[j];
					else
						s  << ", " << ((float*)(option->value))[j];
				}
			} break;
			case CL_PROGRAM_OPTION_BOOL: {
				for(unsigned int j = 0; j < option->count; j++) {
					if(j == 0)
						s << (((bool*)(option->value))[j]? "y" : "n");
					else
						s << ", " << (((bool*)(option->value))[j]? "y" : "n");
				}
			} break;
			default: {					
				for(unsigned int j = 0; j < option->count; j++) {
					if(j == 0)
						s << ((char**)(option->value))[j];
					else
						s  << ", " << ((char**)(option->value))[j];
				}
			} break;
		}
		s.flush();
		std::string final = s.str();
		*value = (char*)malloc(final.length() + 1);
		strncpy(*value, final.c_str(), final.length());
		(*value)[final.length()] = 0;
	}
}


void clHandleProgramOptions(
	CLProgramOption* options[], 
	unsigned int num_options) {

	for(unsigned int i = 0; i < num_options; i++) {
		std::cout << options[i]->name << "";
		char* str_option = NULL;
		printProgramOptionValue(options[i], &str_option);		
		if(str_option != NULL) {
			std::cout << " [" << str_option << "]";
			free(str_option);
		}
		std::string temp;
		if(options[i]->variant == CL_PROGRAM_OPTION_SINGLE) {
			std::cout << ": ";
			std::getline(std::cin, temp);
			if(temp.length() > 0)
				parseProgramOptionValue(options[i], temp.c_str());
		}
		else {
			std::cout << ": ";
			bool done = false;
			while(!done) {
				std::string temp_sample;
				std::getline(std::cin, temp_sample);
				if(temp_sample.length() > 0) {
					if(temp.length() == 0)
						temp.append(temp_sample);
					else {
						temp.append(",");
						temp.append(temp_sample);
					}
					std::cout << options[i]->name << " [" << temp << "]";
					std::cout << ": ";		
				}
				else
					done = true;
			}			
			if(temp.length() > 0)
				parseProgramOptionValue(options[i], temp.c_str());
		}
	}
}

void clPrintProgramOptions(
	CLProgramOption* options[], 
	unsigned int num_options) {
	
	unsigned int longest_name = 0;
	//Compute longest option name		
	for(unsigned int i = 0; i < num_options; i++) {
		if(strlen(options[i]->name) > longest_name)
			longest_name = strlen(options[i]->name);
	}

	for(unsigned int i = 0; i < num_options; i++) {
		std::cout << "  " << options[i]->name;
		for(unsigned int j = strlen(options[i]->name); j < longest_name + 5; j++)
			std::cout << ".";
	
		char* str_option = NULL;
		printProgramOptionValue(options[i], &str_option);		
		if(str_option != NULL) {
			std::cout << str_option;
			free(str_option);
		}
		std::cout << std::endl;
	}
}


void clCreateProgramOption(
	char* name, 
	char* keyword, 
	CLProgramOptionType type,
	CLProgramOptionVariant variant,
	const char* value,
	unsigned int count,
	CLProgramOption* option) {
		option->name = (char*)malloc(strlen(name) + 1);
		strncpy(option->name, name, strlen(name));
		option->name[strlen(name)] = 0;
		
		option->keyword = (char*)malloc(strlen(keyword) + 1);
		strncpy(option->keyword, keyword, strlen(keyword));
		option->keyword[strlen(keyword)] = 0;

		option->type = type;
		option->variant = variant;
		if(option->variant == CL_PROGRAM_OPTION_SINGLE)
			option->count = 1;
		if(value != NULL)
			parseProgramOptionValue(option, value);
		else
			option->value = NULL;
		option->count = count;
}

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
	const char* def_append_to_file,
	CLProgramOption* test_host,
	const char* def_test_host,
	CLProgramOption* test_devices,
	const char* def_test_devices,
	CLProgramOption* min_device_count,
	const char* def_min_device_count,
	CLProgramOption* max_device_count,
	const char* def_max_device_count) {
		
	clCreateProgramOption(
		"Test host",
		"host",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		def_test_host,
		1,
		test_host);
	
	clCreateProgramOption(
		"Test OpenCL devices",
		"devices",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		def_test_devices,
		1,
		test_devices);
		
	clCreateProgramOption(
		"Minimum number of devices",
		"mindev",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		def_min_device_count,
		1,
		min_device_count);
	
	clCreateProgramOption(
		"Maximum number of devices",
		"maxdev",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		def_max_device_count,
		1,
		max_device_count);

	clCreateProgramOption(
		"Kernel file path",
		"kfile",
		CL_PROGRAM_OPTION_STRING,
		CL_PROGRAM_OPTION_SINGLE,
		def_kernel_path,
		1,
		kernel_path);
	
	clCreateProgramOption(
		"Kernel functions",
		"kfunc",
		CL_PROGRAM_OPTION_STRING,
		CL_PROGRAM_OPTION_MULTIPLE,
		def_kernel_functions,
		def_kernel_functions_count,
		kernel_functions);
	
	clCreateProgramOption(
		"Program build options",
		"kopts",
		CL_PROGRAM_OPTION_STRING,
		CL_PROGRAM_OPTION_SINGLE,
		def_kernel_options,
		1,
		kernel_options);
	
	clCreateProgramOption(
		"Enable kernel profiling",
		"kprof",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		def_kernel_profiling,
		1,
		kernel_profiling);
	
	clCreateProgramOption(
		"Work-group size",
		"wgsze",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		def_local_size,
		1,
		local_size);
	
	clCreateProgramOption(
		"Log file path",
		"lgfle",
		CL_PROGRAM_OPTION_STRING,
		CL_PROGRAM_OPTION_SINGLE,
		def_log_file,
		1,
		log_file);

	clCreateProgramOption(
		"Append to data log file (if exists)",
		"append",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		def_append_to_file,
		1,
		append_to_file);
}

void clFreeProgramOption(
	CLProgramOption* option) {

	free(option->keyword);
	free(option->name);
	if(option->value != NULL) {
		switch(option->type) {
			case CL_PROGRAM_OPTION_INT: {
				free((int*)option->value);
			} break;
			case CL_PROGRAM_OPTION_FLOAT: {
				free((float*)option->value);
			} break;
			case CL_PROGRAM_OPTION_BOOL: {
				free((bool*)option->value);
			} break;
			default: {
				for(unsigned int i = 0; i < option->count; i++) {
					if(((char**)option->value)[i] != NULL)
						free(((char**)option->value)[i]);
				}
				free((char**)option->value);
			} break;
		}
	}
}

const char* clProgramOptionAsString(CLProgramOption* option) {
	if(option->value == NULL)
		return NULL;
	return ((const char**)option->value)[0];
}

int clProgramOptionAsInt(CLProgramOption* option) {
	return ((int*)option->value)[0];
}

float clProgramOptionAsFloat(CLProgramOption* option) {
	return ((float*)option->value)[0];
}

bool clProgramOptionAsBool(CLProgramOption* option) {
	return ((bool*)option->value)[0];
}

const char** clProgramOptionAsStringArray(
	CLProgramOption* option) {
	if(option->value == NULL)
		return NULL;
	return (const char**)option->value;
}

int* clProgramOptionAsIntArray(
	CLProgramOption* option) {
	if(option->value == NULL)
		return NULL;
	return (int*)option->value;
}

float* clProgramOptionAsFloatArray(
	CLProgramOption* option) {
	if(option->value == NULL)
		return NULL;
	return (float*)option->value;
}

bool* clProgramOptionAsBoolArray(
	CLProgramOption* option) {
	return (bool*)option->value;
}
