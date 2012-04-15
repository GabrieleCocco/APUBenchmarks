#include "saxpy.h"

void scatterWork(
	CLDeviceInfo* devices, 
	unsigned int device_count, 
	unsigned int data_size,
	unsigned int preferred_local_size,
	unsigned int cpu_core_count,
	unsigned int* data_sizes, 
	unsigned int* global_sizes,
	unsigned int* local_sizes) {

	/* Compute number of elements per device */
	unsigned int elements_per_device = data_size / device_count;
	for(unsigned int i = 0; i < device_count; i++) 
		data_sizes[i] = elements_per_device;

	/* Adjust the elements so if data_size cannot be divided by device_count */
	unsigned int remaining_elements = data_size % device_count;
	unsigned int current_index = 0;
	while(remaining_elements > 0) {
		data_sizes[current_index]++;
		remaining_elements--;
		current_index = (current_index + 1) / device_count;
	}

	/* Set global sizes so that is is multiple of the local size */
	for(unsigned int i = 0; i < device_count; i++) {
		global_sizes[i] = clRoundUp(data_sizes[i], preferred_local_size);
		local_sizes[i] = preferred_local_size;
	}

	/* 
	unsigned int cpu_index = -1;
	for(unsigned int i = 0; i < device_count; i++) {
		if(devices[i].type == CL_DEVICE_TYPE_CPU) {
			cpu_index = i;
			break;
		}
	}
	if(cpu_index >= 0) {
		local_sizes[cpu_index] = cpu_core_count - device_count;
		global_sizes[cpu_index] = clRoundUp(data_sizes[cpu_index], cpu_core_count - device_count);
	}*/
}

int main(int argc, char* argv[])
{	
	std::cout << "----- Saxpy test start -----" << std::endl;
	
	clCreateDefaultProgramOptions(
		&kernel_path, "saxpy.cl",
		&kernel_functions, "saxpy,saxpy4", 2,
		&kernel_options, "",
		&kernel_profiling, "n",
		&local_size_opt, "128",
		&log_file, "Saxpy_results.csv",
		&append_to_file, "n",
		&test_host, "y",
		&test_devices, "y",
		&min_devices, "1",
		&max_devices, "3");

	/* Handle program options */
	clCreateProgramOption(
		"Scalar multiplier (a in ax * y)", "a",
		CL_PROGRAM_OPTION_FLOAT, CL_PROGRAM_OPTION_SINGLE,
		"2.0", 1,
		&vector_multiplier);

	clCreateProgramOption(
		"Min vector size (Bytes)", "minvs",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"8388608",	1,
		&min_vector_size);

	clCreateProgramOption(
		"Max vector size (Bytes)", "maxvs",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"67108864",	1,
		&max_vector_size);

	clCreateProgramOption(
		"Tries", "tries",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"1", 1,
		&tries);
		
	clCreateProgramOption(
		"Test duration (0 to use tries samples)", "tlapse",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"0", 1,
		&test_duration);

	clCreateProgramOption(
		"Milliseconds to wait between tries", "wait",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"500", 1,
		&wait);

	clCreateProgramOption(
		"Vector size multiplier", "multi",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"2", 1,
		&increment);
	
	clCreateProgramOption(
		"Enable float4", "float4",
		CL_PROGRAM_OPTION_BOOL, CL_PROGRAM_OPTION_SINGLE,
		"n", 1,
		&test_float_4);
		
	clCreateProgramOption(
		"Verify output", "verify",
		CL_PROGRAM_OPTION_BOOL,	CL_PROGRAM_OPTION_SINGLE,
		"y", 1,
		&verify_output);

	clCreateProgramOption(
		"Number of native thread (0 for auto)", "cputhreads",
		CL_PROGRAM_OPTION_INT, CL_PROGRAM_OPTION_SINGLE,
		"4", 1,
		&cpu_threads);
	
	CLProgramOption* options[] = { 
		&kernel_path,
		&kernel_functions,
		&kernel_options,
		&kernel_profiling,
		&local_size_opt,
		&cpu_threads,
		&min_vector_size, 
		&max_vector_size,
		&increment, 
		&tries,
		&test_duration,
		&wait,
		&vector_multiplier,
		&test_float_4,
		&test_host,
		&test_devices,
		&min_devices,
		&max_devices,
		&verify_output,
		&log_file,
		&append_to_file };
	
	unsigned int option_count = 21;
	clHandleProgramOptions(options, option_count);
	std::cout << std::endl << "- Program options summarized below" << std::endl;
	clPrintProgramOptions(options, option_count);

	/* Create memory settings */
	CLMemorySetting device_copy[2];
	clCreateMemorySetting("X device copy", CL_MEM_READ_ONLY, 0, &device_copy[0]);
	clCreateMemorySetting("Y device copy", CL_MEM_READ_WRITE, 0, &device_copy[1]);
	CLMemorySetting host_map[2];
	clCreateMemorySetting("X host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[0]);
	clCreateMemorySetting("Y host map", CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[1]);
	
#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	CLMemorySetting special_map[2];
	clCreateMemorySetting("X special map", CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[0]);
	clCreateMemorySetting("Y special map", CL_MEM_READ_WRITE | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[1]);
	CLMemorySetting host_map_special_map[2];
	clCreateMemorySetting("X host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map_special_map[0]);
	clCreateMemorySetting("Y special map", CL_MEM_READ_WRITE | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &host_map_special_map[1]);
#endif

#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	CLMemorySetting* settings[4] = {
		device_copy,
		host_map,
		special_map,
		host_map_special_map };
	unsigned int memset_count = 4;
#else
	CLMemorySetting* settings[2] = {
		device_copy,
		host_map };
	unsigned int memset_count = 2;
#endif
	
	/* Compute number of cores */	
	unsigned int host_cores = clCpuCoreCount();
	if(clProgramOptionAsInt(&cpu_threads) > 0)
		host_cores = clProgramOptionAsInt(&cpu_threads);
	std::cout << std::endl << "- Compute number of CPU cores..." << clCpuCoreCount() << " (will use " << host_cores << ")" << std::endl;
	
	/* Discover devices */
	std::cout << std::endl << "- Tested devices listed below" << std::endl;
	unsigned int dcount;
	CLDeviceInfo* devices;
	clListDevices(&dcount, &devices);
	clPrintDeviceList(devices, dcount, "  ");
	
	/* Open log file */
	std::cout << std::endl << "- Setting log file...";
	std::ofstream* output;
	if(clProgramOptionAsBool(&append_to_file))
		output = new std::ofstream(clProgramOptionAsString(&log_file), std::ios::app);
	else
		output = new std::ofstream(clProgramOptionAsString(&log_file), std::ios::trunc);
	std::cout << "DONE!" << std::endl;

	/* Set up data struct required by computations */
	SaxpyProfilingData pf_data;
	pf_data.a = clProgramOptionAsFloat(&vector_multiplier);
	pf_data.verify_output = clProgramOptionAsBool(&verify_output);

	/* CPU sequential */	
	if(clProgramOptionAsBool(&test_host)) {
		for(unsigned int cores = 1; cores <= host_cores; cores += host_cores - 1) {
			std::cout << std::endl << "- Testing CPU (" << cores << " cores)..." << std::endl;
			*output << "CPU (" << cores << " cores)" << std::endl;
			*output << "Vector size (bytes);Samples;Start time;End time;Duration" << std::endl;

			for(unsigned int vector_size = (unsigned int)clProgramOptionAsInt(&min_vector_size); 
				vector_size <= (unsigned int)clProgramOptionAsInt(&max_vector_size);
				vector_size *= (unsigned int)clProgramOptionAsInt(&increment)) 
			{
				printf("- Vector size %10d bytes...", vector_size);
				
				/* Set up sizes */
				unsigned int global_size = cores;
				unsigned int data_size = vector_size / sizeof(float);

				/* Compute reference if needed */
				float* reference = (float*)malloc(vector_size);	
				if(clProgramOptionAsBool(&verify_output)) {
					float* x = (float*)malloc(vector_size);
					float* y = (float*)malloc(vector_size);
					computeInput(x, y, data_size);
					computeOutput(x, y, reference, clProgramOptionAsFloat(&vector_multiplier), data_size);
					free(x);
					free(y);
				}
		
				SYSTEMTIME start, end;
				unsigned int samples;
				pf_data.reference = reference;
				pf_data.global_sizes = &global_size;
				pf_data.data_sizes = &data_size;
			
				CLProfilingResult result = clProfileComputation(
					runHostComputation, 
					&pf_data,
					clProgramOptionAsInt(&tries),
					clProgramOptionAsInt(&test_duration),
					CL_PROFILING_RESULT_AVERAGE,
					&start, &end, &samples);
		
				*output << vector_size << ";" << samples << ";";
				*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
				*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
				*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
				*output << std::endl;
		
				printf("%*.*lf ms (%6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
	
				free(reference);

				unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
				Sleep(to_wait);
			} 
			*output << std::endl;
		}
	}
	
	/* OpenCL for each OpenCL device */
	if(clProgramOptionAsBool(&test_devices)) {
		for(unsigned int device_count = clProgramOptionAsInt(&min_devices); device_count <= clProgramOptionAsInt(&max_devices); device_count++)  {
			unsigned int combination_count = clCombination(dcount, device_count);
			unsigned int** combinations = clCreateCombination(dcount, 0, device_count);
		
			for(unsigned int combination_index = 0; combination_index < combination_count; combination_index++) {
				unsigned int* current_combination = combinations[combination_index];

				std::cout << std::endl << "- Testing following devices" << std::endl;
				for(unsigned int i = 0; i < device_count; i++) {
					char* type;
					clDeviceTypeToString(devices[current_combination[i]].type, &type);
					std::cout << "    " << devices[current_combination[i]].name << "[" << type << "]..." << std::endl;
					*output << devices[current_combination[i]].name << "(" << type << ")";
					free(type);
				}
				std::cout << std::endl;
				*output << std::endl;

				/* Create opencl environment for each device */
				std::cout << "- Creating opencl environment...";
				CLDeviceEnvironment* environments = (CLDeviceEnvironment*)malloc(device_count * sizeof(CLDeviceEnvironment));
				CLDeviceInfo* selected_devices = (CLDeviceInfo*)malloc(device_count * sizeof(CLDeviceInfo));
				for(unsigned int i = 0; i < device_count; i++)
					selected_devices[i] = devices[current_combination[i]];
				clCreateDeviceEnvironment(
					selected_devices,
					device_count,
					clProgramOptionAsString(&kernel_path),
					clProgramOptionAsStringArray(&kernel_functions),
					kernel_functions.count,
					clProgramOptionAsString(&kernel_options),
					clProgramOptionAsBool(&kernel_profiling),
					false,
					environments);
				
				pf_data.environments = environments;
				pf_data.device_count = device_count;
				
				std::cout << std::endl;
				for(unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {						
					printf("  %-15s - %15s\n", settings[memset_index][0].name, settings[memset_index][1].name);
					*output << settings[memset_index][0].name << "-" << settings[memset_index][1].name << std::endl;
					*output << "Vector size (bytes);Samples;Start time;End time;Duration";
			
					if(clProgramOptionAsBool(&test_float_4)) 
						*output << ";;Samples (float 4);Start time (float 4);End time (float 4);Duration (float 4)";
					*output << std::endl;
				
					for(unsigned int vector_size = (unsigned int)clProgramOptionAsInt(&min_vector_size); 
						vector_size <= (unsigned int)clProgramOptionAsInt(&max_vector_size);
						vector_size *= (unsigned int)clProgramOptionAsInt(&increment)) 
					{
						printf("- Vector size %10d bytes...", vector_size);

						/* Set up sizes */
						unsigned int global_sizes[MAX_DEV_COUNT];
						unsigned int local_sizes[MAX_DEV_COUNT];
						unsigned int data_sizes[MAX_DEV_COUNT];
						unsigned int kernel_index[MAX_DEV_COUNT];
						scatterWork(
							selected_devices, 
							device_count, 
							vector_size / sizeof(float), 
							clProgramOptionAsInt(&local_size_opt),
							host_cores,
							data_sizes, global_sizes, local_sizes);
						for(unsigned int i = 0; i < device_count; i++) {
						//	global_sizes[i] = vector_size / sizeof(float) / device_count;
							//local_sizes[i] = clProgramOptionAsInt(&local_size_opt);
							//data_sizes[i] = global_sizes[i];
							kernel_index[i] = 0;
						}

						/* Compute reference if needed */
						float* reference = (float*)malloc(vector_size);
						if(clProgramOptionAsBool(&verify_output)) {
							float* x = (float*)malloc(vector_size);
							float* y = (float*)malloc(vector_size);
							computeInput(x, y, vector_size / sizeof(float));
							computeOutput(x, y, reference, clProgramOptionAsFloat(&vector_multiplier), vector_size / sizeof(float));
							free(x);
							free(y);
						}

						SYSTEMTIME start, end;
						unsigned int samples;			
						pf_data.global_sizes = global_sizes;
						pf_data.local_sizes = local_sizes;
						pf_data.data_sizes = data_sizes;
						pf_data.kernel_index = kernel_index;
						pf_data.type_size = 1;
						pf_data.src_flags = settings[memset_index][0].flags;
						pf_data.dst_flags = settings[memset_index][1].flags;
						pf_data.map_src = settings[memset_index][0].mapping;
						pf_data.map_dst = settings[memset_index][1].mapping;
						pf_data.reference = reference;

						CLProfilingResult result;
						if(device_count == 1)
							result = clProfileComputation(
								runDeviceComputation, 
								&pf_data,
								clProgramOptionAsInt(&tries),
								clProgramOptionAsInt(&test_duration),
								CL_PROFILING_RESULT_AVERAGE,
								&start, &end, &samples);
						else
							result = clProfileComputation(
								runHeteroSeparatedComputation, 
								&pf_data,
								clProgramOptionAsInt(&tries),
								clProgramOptionAsInt(&test_duration),
								CL_PROFILING_RESULT_AVERAGE,
								&start, &end, &samples);

						*output << vector_size << ";" << samples << ";";
						*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
						*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
						*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
						*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;

						printf("%*.*lf ms (float1, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
		
						unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
						Sleep(to_wait);
				
						if(clProgramOptionAsBool(&test_float_4)) {			
							/* Set up sizes */	
							scatterWork(
								selected_devices, 
								device_count, 
								vector_size / sizeof(float) / 4, 
								clProgramOptionAsInt(&local_size_opt),
								host_cores,
								data_sizes, global_sizes, local_sizes);
							for(unsigned int i = 0; i < device_count; i++) {
								//pf_data.global_sizes[i] = vector_size / sizeof(float) / device_count / 4;
								//pf_data.data_sizes[i] = pf_data.global_sizes[i];
								pf_data.kernel_index[i] = 1;
							}
							pf_data.type_size = 4;

							if(device_count == 1)
								result = clProfileComputation(
									runDeviceComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);
							else
								result = clProfileComputation(
									runHeteroSeparatedComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);
					
							*output << ";;" << samples << ";";
							*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
							*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
							*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
							*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
							printf("                                 ");
							printf("%*.*lf ms (float4, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
							
							unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
							Sleep(to_wait);
						}
						free(reference);

						*output << std::endl;
					}
				}				
				clFreeDeviceEnvironments(environments, device_count, false);
				free(environments);
				free(selected_devices);
			}
			for(unsigned int i = 0; i < combination_count; i++)
				free(combinations[i]);
			free(combinations);
			*output << std::endl;
		}
		*output << std::endl;
	}

	/* Free resources */
	std::cout << std::endl << "- Clear environment, devices and options...";
	output->flush();
	output->close();
	delete output;
	
	for(unsigned int i = 0; i < memset_count; i++) {
		clFreeMemorySetting(settings[i]);
	}
	for(unsigned int i = 0; i < dcount; i++) {
		clFreeDeviceInfo(&devices[i]);
	}
	free(devices);

	for(unsigned int i = 0; i < option_count; i++)
		clFreeProgramOption(options[i]);
	std::cout << "DONE!" << std::endl;
	
	std::cout << "----- Saxpy test end - press any key to exit -----" << std::endl;
	getchar();
}

