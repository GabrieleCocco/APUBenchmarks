#include "matrix_mult.h"
#include <time.h>


// Globals for size of matrices
//unsigned int uiWA, uiHA, uiWB, uiHB, uiWC, uiHC;


int main(int argc, char* argv[])
{	
	std::cout << "----- MatrixMult test start -----" << std::endl;
	
	clCreateDefaultProgramOptions(
		&kernel_path, "matrix_mult.cl",
		&kernel_functions, "matrixMul,matrixMulBase", 2,
		&kernel_options, "",
		&kernel_profiling, "n",
		&local_size, "128",
		&log_file, "MatrixMult_results.csv",
		&append_to_file, "n");

	/* Handle program options */
	clCreateProgramOption(
		"Min matrix side (must be a multiple of block side)",
		"mins",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"16",
		1,
		&min_matrix_size);

	clCreateProgramOption(
		"Max matrix side (must be a multiple of block side)",
		"maxs",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"512",
		1,
		&max_matrix_size);
	
	clCreateProgramOption(
		"Block side",
		"blocks",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"16",
		1,
		&block_size);

	clCreateProgramOption(
		"Tries",
		"tries",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"1",
		1,
		&tries);
		
	clCreateProgramOption(
		"Test duration (0 to use tries samples)",
		"tlapse",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"0",
		1,
		&test_duration);

	clCreateProgramOption(
		"Milliseconds to wait between tries",
		"wait",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"500",
		1,
		&wait);

	clCreateProgramOption(
		"Matrix size multiplier",
		"multi",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"2",
		1,
		&size_multiplier);
	
	clCreateProgramOption(
		"Enable float4",
		"ls",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"n",
		1,
		&test_float_4);
		
	clCreateProgramOption(
		"Test unoptimized version",
		"unopt",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"n",
		1,
		&test_unoptimized);

	clCreateProgramOption(
		"Verify output",
		"verify",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"y",
		1,
		&verify_output);

	clCreateProgramOption(
		"Simulate host result usage",
		"useresult",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"n",
		1,
		&simulate_read);
		
	clCreateProgramOption(
		"Number of native thread (0 for auto)",
		"cputhreads",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"4",
		1,
		&cpu_threads);
		
	clCreateProgramOption(
		"Test CPU (sequential)",
		"cpuseq",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"y",
		1,
		&test_cpu_sequential);
	
	clCreateProgramOption(
		"Test CPU (native threads)",
		"cputhreads",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"y",
		1,
		&test_cpu_threads);
	
	clCreateProgramOption(
		"Test OpenCL devices",
		"ocldev",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"y",
		1,
		&test_opencl_devices);
	
	clCreateProgramOption(
		"Test heterogeneous (CPU + GPU)",
		"hetero",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"y",
		1,
		&test_heterogeneous);

	CLProgramOption* options[] = { 
		&kernel_path,
		&kernel_functions,
		&kernel_options,
		&kernel_profiling,
		&local_size,
		&cpu_threads,
		&min_matrix_size, 
		&max_matrix_size,
		&block_size,
		&size_multiplier,
		&tries,
		&test_duration,
		&wait,
		&test_float_4,
		&test_unoptimized,
		&test_cpu_sequential,
		&test_cpu_threads,
		&test_opencl_devices,
		&test_heterogeneous,
		&simulate_read,
		&verify_output,
		&log_file,
		&append_to_file };
	
	unsigned int option_count = 23;
	clHandleProgramOptions(options, option_count);
	std::cout << std::endl << "- Program options summarized below" << std::endl;
	clPrintProgramOptions(options, option_count);

	/* Create memory settings */
	CLMemorySetting device_copy[2];
	clCreateMemorySetting("A,B device copy", CL_MEM_READ_ONLY, 0, &device_copy[0]);
	clCreateMemorySetting("C device copy", CL_MEM_WRITE_ONLY, 0, &device_copy[1]);
	CLMemorySetting host_map[2];
	clCreateMemorySetting("A,B host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[0]);
	clCreateMemorySetting("C host map", CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[1]);
	CLMemorySetting host_map_device_copy[2];
	clCreateMemorySetting("A,B host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map_device_copy[0]);
	clCreateMemorySetting("C device copy", CL_MEM_WRITE_ONLY, 0, &host_map_device_copy[1]);
	
#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD/*
	CLMemorySetting special_map[2];
	clCreateMemorySetting("A,B special map", CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[0]);
	clCreateMemorySetting("C special map", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[1]); */
	CLMemorySetting special_copy[2];
	clCreateMemorySetting("A,B special copy", CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 0, &special_copy[0]);
	clCreateMemorySetting("C special copy", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 0, &special_copy[1]);
	CLMemorySetting host_map_special_map[2];
	clCreateMemorySetting("A,B host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map_special_map[0]);
	clCreateMemorySetting("C special map", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &host_map_special_map[1]);
#endif

#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	CLMemorySetting* settings[1] = {
		special_copy };
	unsigned int memset_count = 1;
#else
	CLMemorySetting* settings[3] = {
		device_copy,
		host_map,
		host_map_device_copy };
	unsigned int memset_count = 3;
#endif

	/* Compute number of cores */	
	unsigned int numCPU = clCpuCoreCount();
	if(clProgramOptionAsInt(&cpu_threads) > 0)
		numCPU = clProgramOptionAsInt(&cpu_threads);
	std::cout << std::endl << "- Compute number of CPU cores..." << clCpuCoreCount() << " (will use " << numCPU << ")" << std::endl;

	/* Discover devices */
	std::cout << std::endl << "- Tested devices listed below" << std::endl;
	unsigned int device_count;
	CLDevice* devices;
	clListDevices(&device_count, &devices);
	clPrintDeviceList(devices, device_count, "  ");

	/* Create opencl environment for each device */
	std::cout << std::endl << "- Creating opencl environment for each tested device...";
	char buildOptions[256];
	memset(buildOptions, 0, 256);
	sprintf(
		buildOptions, "-D BLOCK_SIZE=%d %s", 
		clProgramOptionAsInt(&block_size),
		clProgramOptionAsString(&kernel_options) != NULL ? clProgramOptionAsString(&kernel_options) : "");

	CLEnvironment* environments = (CLEnvironment*)malloc(device_count * sizeof(CLEnvironment));
	for(unsigned int i = 0; i < device_count; i++) {
		clCreateEnvironment(
			&devices[i],
			clProgramOptionAsString(&kernel_path),
			clProgramOptionAsStringArray(&kernel_functions),
			kernel_functions.count,
			buildOptions,
			clProgramOptionAsBool(&kernel_profiling),
			&environments[i]);
	}

	/* Open log file */
	std::cout << std::endl << "- Setting log file...";
	std::ofstream* output;
	if(clProgramOptionAsBool(&append_to_file))
		output = new std::ofstream(clProgramOptionAsString(&log_file), std::ios::app);
	else
		output = new std::ofstream(clProgramOptionAsString(&log_file), std::ios::trunc);
	std::cout << "DONE!" << std::endl;
	
	/* Set up data struct required by computations */
	MatrixMultProfilingData pf_data;
	pf_data.local_size = clProgramOptionAsInt(&local_size);
	pf_data.verify_output = clProgramOptionAsBool(&verify_output);
	pf_data.src_flags = 0;
	pf_data.dst_flags = 0;
	pf_data.env = NULL;
	pf_data.kernel = NULL;
	pf_data.map_dst = 0;
	pf_data.map_src = 0;
	pf_data.block_size = clProgramOptionAsInt(&block_size);
	pf_data.cpu_threads = 1;

	/* CPU sequential */	
	if(clProgramOptionAsBool(&test_cpu_sequential)) {
		std::cout << std::endl << "- Testing CPU sequential..." << std::endl;
		*output << "CPU sequential" << std::endl;
		*output << "Matrix size (elements);Samples;Start time;End time;Duration;Alloc time;Init time;Exec time;Read time" << std::endl;

		unsigned int i = 0;
		for(unsigned int height_a = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
			height_a <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
			height_a *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
		{
			for(unsigned int other_side_mult = 1; other_side_mult <= 2 && (other_side_mult * height_a < clProgramOptionAsInt(&max_matrix_size)); other_side_mult++) {
				unsigned int width_a = other_side_mult * height_a;
				
				i++;
				printf("%2d) Matrix %4d x %4d (%8d bytes)...", i, width_a, height_a, width_a * height_a * sizeof(float));

				SYSTEMTIME start, end;
				unsigned int samples;
				pf_data.width_a = width_a;
				pf_data.height_a = height_a;
			
				CLProfilingResult result = clProfileComputation(
					runHostComputation, 
					&pf_data,
					clProgramOptionAsInt(&tries),
					clProgramOptionAsInt(&test_duration),
					CL_PROFILING_RESULT_AVERAGE,
					&start, &end, &samples);
		
				if(result.success) {
					*output << width_a << " x " << height_a << ";" << samples << ";";
					*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
					*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
					*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
					*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
					*output << std::endl;
		
					printf("%*.*lf ms (%6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
				}			
				else
					printf("  PROFILING FAIL!\n");
				unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
				Sleep(to_wait);
			}
		} 
	}
	*output << std::endl;
	
	/* CPU native threads */
	if(clProgramOptionAsBool(&test_cpu_threads)) {
		std::cout << std::endl << "- Testing CPU native threads (" << numCPU << " threads)..." << std::endl;
		*output << "CPU native threads" << std::endl;
		*output << "Matrix size (elements);Samples;Start time;End time;Duration;Alloc time;Init time;Exec time;Read time" << std::endl;

		unsigned int i = 0;
		for(unsigned int height_a = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
			height_a <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
			height_a *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
		{
			for(unsigned int other_side_mult = 1; other_side_mult <= 2 && (other_side_mult * height_a < clProgramOptionAsInt(&max_matrix_size)); other_side_mult++) {
				unsigned int width_a = other_side_mult * height_a;
								
				i++;
				printf("%2d) Matrix %4d x %4d (%8d bytes)...", i, width_a, height_a, width_a * height_a * sizeof(float));

				SYSTEMTIME start, end;
				unsigned int samples;
				pf_data.width_a = width_a;
				pf_data.height_a = height_a;
				pf_data.block_size = 
				pf_data.cpu_threads = numCPU;
			
				CLProfilingResult result = clProfileComputation(
					runHostComputation, 
					&pf_data,
					clProgramOptionAsInt(&tries),
					clProgramOptionAsInt(&test_duration),
					CL_PROFILING_RESULT_AVERAGE,
					&start, &end, &samples);
		
				if(result.success) {
					*output << width_a << " x " << height_a << ";" << samples << ";";
					*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
					*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
					*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
					*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
					*output << std::endl;
		
					printf("%*.*lf ms (%6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
				}			
				else
					printf("  PROFILING FAIL!\n");

				unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
				Sleep(to_wait);
			}
		} 
	}
	*output << std::endl;

	/* OpenCL for each OpenCL device */
	if(clProgramOptionAsBool(&test_opencl_devices)) {
		for(unsigned int device_index = 0; device_index < device_count; device_index++) {
			char* type;
			clDeviceTypeToString(devices[device_index].type, &type);
			std::cout << std::endl << "- Testing " << devices[device_index].name << "[" << type << "]..." << std::endl;
			*output << devices[device_index].name << " (" << type << ")" << std::endl;
			free(type);
		
			for(unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {						
				printf("%-15s - %15s\n", settings[memset_index][0].name, settings[memset_index][1].name);
				*output << settings[memset_index][0].name << "-" << settings[memset_index][1].name << std::endl;
				*output << "Matrix side (elements);Samples;Start time;End time;Duration;Alloc time;Init time;Exec time;Read time";
			
				if(clProgramOptionAsBool(&test_unoptimized)) 
					*output << ";;Samples (unoptimized);Start time (unoptimized);End time (unoptimized);Duration (unoptimized);Alloc time (unoptimized);Init time (unoptimized);Exec time (unoptimized);Read time (unoptimized)";
				if(clProgramOptionAsBool(&test_float_4)) 
					*output << ";;Samples (float 4);Start time (float 4);End time (float 4);Duration (float 4);Alloc time (float 4);Init time (float 4);Exec time (float 4);Read time (float 4)";
				*output << std::endl;
				
				unsigned int i = 0;
				for(unsigned int height_a = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
					height_a <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
					height_a *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
				{					
					for(unsigned int other_side_mult = 1; other_side_mult <= 2 && (other_side_mult * height_a < clProgramOptionAsInt(&max_matrix_size)); other_side_mult++) {
						unsigned int width_a = other_side_mult * height_a;

						i++;
						printf("%2d) Matrix %4d x %4d (%8d bytes)...", i, width_a, height_a, width_a * height_a * sizeof(float));
						
						SYSTEMTIME start, end;
						unsigned int samples;
						pf_data.width_a = width_a;
						pf_data.height_a = height_a;
						pf_data.env = &environments[device_index];
						pf_data.kernel = environments[device_index].kernels[0];
						pf_data.src_flags = settings[memset_index][0].flags;
						pf_data.dst_flags = settings[memset_index][1].flags;
						pf_data.map_src = settings[memset_index][0].mapping;
						pf_data.map_dst = settings[memset_index][1].mapping;
			
						CLProfilingResult result = clProfileComputation(
							runGpuComputation, 
							&pf_data,
							clProgramOptionAsInt(&tries),
							clProgramOptionAsInt(&test_duration),
							CL_PROFILING_RESULT_AVERAGE,
							&start, &end, &samples);
		
						if(result.success) {
							*output << width_a << " x " << height_a << ";" << samples << ";";
							*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
							*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
							*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
							*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
							printf("%*.*lf ms (float1, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
						}			
						else
							printf("  PROFILING FAIL!\n");

						unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
						Sleep(to_wait);
			
						if(clProgramOptionAsBool(&test_unoptimized)) {
							pf_data.kernel = environments[device_index].kernels[1];							
							result = clProfileComputation(
								runGpuComputation, 
								&pf_data,
								clProgramOptionAsInt(&tries),
								clProgramOptionAsInt(&test_duration),
								CL_PROFILING_RESULT_AVERAGE,
								&start, &end, &samples);
		
							if(result.success) {
								*output << width_a << " x " << height_a << ";" << samples << ";";
								*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
								*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
								*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
								*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
								printf("                                          ");
								printf("%*.*lf ms (unopti, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
							}			
							else
								printf("  PROFILING FAIL!\n");

							unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
							Sleep(to_wait);
						}

						if(/*clProgramOptionAsBool(&test_float_4)*/false) {
							pf_data.kernel = environments[device_index].kernels[2];							
							result = clProfileComputation(
								runGpuComputation, 
								&pf_data,
								clProgramOptionAsInt(&tries),
								clProgramOptionAsInt(&test_duration),
								CL_PROFILING_RESULT_AVERAGE,
								&start, &end, &samples);
		
							if(result.success) {
								*output << width_a << " x " << height_a << ";" << samples << ";";
								*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
								*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
								*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
								*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
								printf("                                          ");
								printf("%*.*lf ms (float4, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
							}			
							else
								printf("  PROFILING FAIL!\n");

							unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
							Sleep(to_wait);
						}
						*output << std::endl;
					}
				}
			}
		}
		*output << std::endl;
	}
	
	/* CPU native threads + GPU */
	if(clProgramOptionAsBool(&test_heterogeneous)) {
		for(unsigned int device_index = 0; device_index < device_count; device_index++) {
			if(devices[device_index].type != CL_DEVICE_TYPE_CPU) {
				char* type;
				clDeviceTypeToString(devices[device_index].type, &type);
				std::cout << std::endl << "- Testing CPU native threads + " << devices[device_index].name << "[" << type << "]..." << std::endl;
				*output << "CPU native threads + " << devices[device_index].name << " (" << type << ")" << std::endl;
				free(type);
		
				for(unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {						
					printf("%-15s - %15s\n", settings[memset_index][0].name, settings[memset_index][1].name);
					*output << settings[memset_index][0].name << "-" << settings[memset_index][1].name << std::endl;
					*output << "Matrix side (elements);Samples;Start time;End time;Duration;Alloc time;Init time;Exec time;Read time";
			
					if(clProgramOptionAsBool(&test_unoptimized)) 
						*output << ";;Samples (unoptimized);Start time (unoptimized);End time (unoptimized);Duration (unoptimized);Alloc time (unoptimized);Init time (unoptimized);Exec time (unoptimized);Read time (unoptimized)";
					if(clProgramOptionAsBool(&test_float_4)) 
						*output << ";;Samples (float 4);Start time (float 4);End time (float 4);Duration (float 4);Alloc time (float 4);Init time (float 4);Exec time (float 4);Read time (float 4)";
					*output << std::endl;
				
					unsigned int i = 0;
					for(unsigned int height_a = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
						height_a <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
						height_a *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
					{					
						for(unsigned int other_side_mult = 1; other_side_mult <= 2 && (other_side_mult * height_a < clProgramOptionAsInt(&max_matrix_size)); other_side_mult++) {
							unsigned int width_a = other_side_mult * height_a;

							i++;
							printf("%2d) Matrix %4d x %4d (%8d bytes)...", i, width_a, height_a, width_a * height_a * sizeof(float));
						
							SYSTEMTIME start, end;
							unsigned int samples;
							pf_data.width_a = width_a;
							pf_data.height_a = height_a;
							pf_data.env = &environments[device_index];
							pf_data.kernel = environments[device_index].kernels[0];
							pf_data.src_flags = settings[memset_index][0].flags;
							pf_data.dst_flags = settings[memset_index][1].flags;
							pf_data.map_src = settings[memset_index][0].mapping;
							pf_data.map_dst = settings[memset_index][1].mapping;
			
							CLProfilingResult result = clProfileComputation(
								runHostGpuComputation, 
								&pf_data,
								clProgramOptionAsInt(&tries),
								clProgramOptionAsInt(&test_duration),
								CL_PROFILING_RESULT_AVERAGE,
								&start, &end, &samples);
		
							if(result.success) {
								*output << width_a << " x " << height_a << ";" << samples << ";";
								*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
								*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
								*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
								*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
								printf("%*.*lf ms (float1, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
							}			
							else
								printf("  PROFILING FAIL!\n");

							unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
							Sleep(to_wait);
			
							if(clProgramOptionAsBool(&test_unoptimized)) {
								pf_data.kernel = environments[device_index].kernels[1];							
								result = clProfileComputation(
									runHostGpuComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);
		
								if(result.success) {
									*output << width_a << " x " << height_a << ";" << samples << ";";
									*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
									*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
									*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
									*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
									printf("                                          ");
									printf("%*.*lf ms (unopti, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
								}			
								else
									printf("  PROFILING FAIL!\n");

								unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
								Sleep(to_wait);
							}

							if(/*clProgramOptionAsBool(&test_float_4)*/false) {
								pf_data.kernel = environments[device_index].kernels[2];							
								result = clProfileComputation(
									runGpuComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);
		
								if(result.success) {
									*output << width_a << " x " << height_a << ";" << samples << ";";
									*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
									*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
									*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
									*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;
		
									printf("                                          ");
									printf("%*.*lf ms (float4, %6d samples)\n", 9, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
								}			
								else
									printf("  PROFILING FAIL!\n");

								unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
								Sleep(to_wait);
							}
							*output << std::endl;
						}
					}
				}
				*output << std::endl;
			}
		}
	}
	
	/* Free resources */
	std::cout << std::endl << "- Clear environment, devices and options...";
	output->flush();
	output->close();
	delete output;
	
	for(unsigned int i = 0; i < memset_count; i++) {
		clFreeMemorySetting(settings[i]);
	}

	for(unsigned int i = 0; i < device_count; i++) {
		clFreeEnvironment(&environments[i]);
		clFreeDevice(&devices[i]);
	}
	free(environments);
	free(devices);

	for(unsigned int i = 0; i < option_count; i++)
		clFreeProgramOption(options[i]);
	std::cout << "DONE!" << std::endl;
	
	std::cout << "----- MatrixMult test end - press any key to exit -----" << std::endl;
	getchar();
}

