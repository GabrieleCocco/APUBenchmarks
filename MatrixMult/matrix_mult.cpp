#include "matrix_mult.h"
#include <time.h>

void scatterWork(
	CLDeviceInfo* devices, 
	double* current_scales,
	unsigned int device_count, 
	unsigned int matrix_width,
	unsigned int matrix_height,
	CLSize2D preferred_local_size,
	unsigned int cpu_core_count,
	CLSize2D* data_sizes, 
	CLSize2D* global_sizes) {

	/* GPU 5x faster than IGX, CPU 50x slower than GPU */
	double total_scale = 0;
	for(unsigned int i = 0; i < device_count; i++) 
		total_scale += current_scales[i];

	/* Compute number of elements per device */
	double rows_per_device = (double)matrix_height / (double)total_scale;
	unsigned int remaining_rows = matrix_height;
	unsigned int rows_max_index = 0;
	for(unsigned int i = 0; i < device_count; i++) {
		data_sizes[i].width = matrix_width;
		//I would like to compute on these rows
		int rows = (int)(rows_per_device * current_scales[i]);
		//But i must respect block_size
		rows = clRoundUp(rows, preferred_local_size.height);
		//Check that rows is not greater than remeining rows
		data_sizes[i].height = min(rows, remaining_rows);
		remaining_rows -= data_sizes[i].height;
		if(data_sizes[i].height > rows_max_index)
			rows_max_index = i;
	}

	//Check i didn't assign 0 rows to anybody
	for(unsigned int i = 0; i < device_count; i++) {
		if(data_sizes[i].height == 0) {
			data_sizes[i].height = preferred_local_size.height;
			//remove block_size rows from who is assigned the max rows count
			data_sizes[rows_max_index].height -= preferred_local_size.height;
		}
	}
	/* Adjust the elements so if data_size cannot be divided by device_count 
	unsigned int current_index = 0;
	while(remaining_rows > 0) {
		data_sizes[current_index].height += min(scale[current_index], remaining_rows);
		remaining_rows -= min(scale[current_index], remaining_rows);
		current_index = (current_index + 1) / device_count;
	} */

	/* Set global sizes so that is is multiple of the local size */
	for(unsigned int i = 0; i < device_count; i++) {
		global_sizes[i].height = clRoundUp(data_sizes[i].height, preferred_local_size.height);
		global_sizes[i].width = clRoundUp(data_sizes[i].width, preferred_local_size.width);
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
	std::cout << "----- MatrixMult test start -----" << std::endl;
	
	clCreateDefaultProgramOptions(
		&kernel_path, "matrix_mult.cl",
		&kernel_functions, "matrixMul,matrixMulBase", 2,
		&kernel_options, "",
		&kernel_profiling, "n",
		&local_size, "16",
		&log_file, "MatrixMult_results.csv",
		&append_to_file, "n",
		&test_host, "n",
		&test_devices, "y",
		&min_devices, "1",
		&max_devices, "2");

	/* Handle program options */
	clCreateProgramOption(
		"Min matrix side (must be a multiple of local size)",
		"mins",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"64",
		1,
		&min_matrix_size);

	clCreateProgramOption(
		"Max matrix side (must be a multiple of local size)",
		"maxs",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"1024",
		1,
		&max_matrix_size);

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
		"Verify output",
		"verify",
		CL_PROGRAM_OPTION_BOOL,
		CL_PROGRAM_OPTION_SINGLE,
		"n",
		1,
		&verify_output);
		
	clCreateProgramOption(
		"Number of native thread (0 for auto)",
		"cputhreads",
		CL_PROGRAM_OPTION_INT,
		CL_PROGRAM_OPTION_SINGLE,
		"4",
		1,
		&cpu_threads);
		
	CLProgramOption* options[] = { 
		&kernel_path,
		&kernel_functions,
		&kernel_options,
		&kernel_profiling,
		&local_size,
		&cpu_threads,
		&min_matrix_size, 
		&max_matrix_size,
		&size_multiplier,
		&tries,
		&test_duration,
		&wait,
		&test_host,
		&test_devices,
		&min_devices,
		&max_devices,
		&verify_output,
		&log_file,
		&append_to_file };
	
	unsigned int option_count = 19;
	clHandleProgramOptions(options, option_count);
	std::cout << std::endl << "- Program options summarized below" << std::endl;
	clPrintProgramOptions(options, option_count);

	/* Create memory settings */
	CLMemorySetting device_copy[2];
	clCreateMemorySetting("A, B device copy", CL_MEM_READ_ONLY, 0, &device_copy[0]);
	clCreateMemorySetting("C device copy", CL_MEM_WRITE_ONLY, 0, &device_copy[1]);
	CLMemorySetting host_map[2];
	clCreateMemorySetting("A, B host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[0]);
	clCreateMemorySetting("C host map", CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map[1]);
	
#ifdef FALSES
	CLMemorySetting special_map[2];
	clCreateMemorySetting("A, B special map", CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[0]);
	clCreateMemorySetting("C special map", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &special_map[1]);
	CLMemorySetting host_map_special_map[2];
	clCreateMemorySetting("A, B host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_map_special_map[0]);
	clCreateMemorySetting("C special map", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &host_map_special_map[1]);
#endif

#ifdef FALSES
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
	
	/* Init performance scales for hetero computations */
	PerformanceScaleInfo performance_scales[MAX_DEV_COUNT];
	for(unsigned int i = 0; i < dcount; i++) {
		performance_scales[i].device = devices[i].id;
		performance_scales[i].completion_time = 1;
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
	char buildOptions[256];
	memset(buildOptions, 0, 256);
	sprintf(
		buildOptions, "-D BLOCK_SIZE=%d %s", 
		clProgramOptionAsInt(&local_size),
		clProgramOptionAsString(&kernel_options) != NULL ? clProgramOptionAsString(&kernel_options) : "");
	
	/* Set up data struct required by computations */
	MatrixMultProfilingData pf_data;
	pf_data.verify_output = clProgramOptionAsBool(&verify_output);
	CLSize2D cpu_size, cpu_data_size;

	/* CPU sequential */	
	if(clProgramOptionAsBool(&test_host)) {
		for(unsigned int cores = 1; cores <= host_cores; cores += host_cores - 1) {
			std::cout << std::endl << "- Testing CPU (" << cores << " cores)..." << std::endl;
			*output << "CPU (" << cores << " cores)" << std::endl;
			*output << "Matrix size (elements);Samples;Start time;End time;Duration" << std::endl;

			for(unsigned int matrix_width = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
				matrix_width <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
				matrix_width *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
			{
				for(unsigned int matrix_height = (matrix_width > (unsigned int)clProgramOptionAsInt(&min_matrix_size) ? matrix_width / 2 : matrix_width);
					matrix_height <= matrix_width;
					matrix_height *= 2) 
				{
					printf("- Matrix size %5d x %5d bytes...", matrix_width, matrix_height);
				
					/* Set up sizes */
					CLSize2D global_size, a_data_size;
					CLSize1D b_data_size;
					global_size.width = 1;
					global_size.height = cores;
					a_data_size.width = matrix_width;
					a_data_size.height = matrix_height;
					b_data_size.width = matrix_height;

					/* Compute reference if needed */
					float* reference = (float*)malloc(matrix_height * matrix_height * sizeof(float));
					if(clProgramOptionAsBool(&verify_output)) {
						float* a = (float*)malloc(matrix_width * matrix_height * sizeof(float));						
						float* b = (float*)malloc(matrix_width * matrix_height * sizeof(float));
						computeInput(a, b, matrix_width, matrix_height, matrix_height, 0);
						computeOutput(a, b, reference, matrix_width, matrix_height, matrix_height);
						free(a);
						free(b);
					}
		
					SYSTEMTIME start, end;
					unsigned int samples;
					pf_data.reference = reference;
					pf_data.global_sizes = &global_size;
					pf_data.a_data_sizes = &a_data_size;
					pf_data.b_data_sizes = &b_data_size;
			
					CLProfilingResult result = clProfileComputation(
						runHostComputation, 
						&pf_data,
						clProgramOptionAsInt(&tries),
						clProgramOptionAsInt(&test_duration),
						CL_PROFILING_RESULT_AVERAGE,
						&start, &end, &samples);
		
					*output << matrix_width << "x" << matrix_height << ";" << samples << ";";
					*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
					*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
					*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
					*output << std::endl;
		
					printf("%*.*lf ms (%6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
	
					free(reference);

					unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
					Sleep(to_wait);
				}
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
				
				double current_scales[MAX_DEV_COUNT];
				//Avoid single CPU opencl device is test host is disabled
				if(device_count == 1 && devices[current_combination[0]].type == CL_DEVICE_TYPE_CPU && !clProgramOptionAsBool(&test_host))
					continue;

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
				for(unsigned int i = 0; i < device_count; i++) {
					selected_devices[i] = devices[current_combination[i]];		
					current_scales[i] = performance_scales[current_combination[i]].completion_time;
				}
				clCreateDeviceEnvironment(
					selected_devices,
					device_count,
					clProgramOptionAsString(&kernel_path),
					clProgramOptionAsStringArray(&kernel_functions),
					kernel_functions.count,
					buildOptions,
					clProgramOptionAsBool(&kernel_profiling),
					false,
					environments);
				
				pf_data.environments = environments;
				pf_data.device_count = device_count;
				
				std::cout << std::endl;

				double average_completion_time = 0;
				unsigned int test_count = 0;

				for(unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {						
					printf("  %-15s - %15s\n", settings[memset_index][0].name, settings[memset_index][1].name);
					*output << settings[memset_index][0].name << "-" << settings[memset_index][1].name << std::endl;
					*output << "Matrix size (elements);Samples;Start time;End time;Duration";

					for(unsigned int matrix_width = (unsigned int)clProgramOptionAsInt(&min_matrix_size); 
						matrix_width <= (unsigned int)clProgramOptionAsInt(&max_matrix_size);
						matrix_width *= (unsigned int)clProgramOptionAsInt(&size_multiplier)) 
					{
						for(unsigned int matrix_height = (matrix_width > (unsigned int)clProgramOptionAsInt(&min_matrix_size) ? matrix_width / 2 : matrix_width);
							matrix_height <= matrix_width;
							matrix_height *= 2) 
						{
							printf("- Matrix size %5d x %5d bytes ", matrix_width, matrix_height);

							/* Set up sizes */
							CLSize2D global_sizes[MAX_DEV_COUNT];
							CLSize1D local_sizes[MAX_DEV_COUNT];
							CLSize2D a_data_sizes[MAX_DEV_COUNT];
							CLSize1D b_data_sizes[MAX_DEV_COUNT];
							unsigned int kernel_index[MAX_DEV_COUNT];

							CLSize2D preferred_local_size;
							preferred_local_size.width = (unsigned int)clProgramOptionAsInt(&local_size);
							preferred_local_size.height = (unsigned int)clProgramOptionAsInt(&local_size);

							scatterWork(
								selected_devices,
								current_scales,
								device_count,
								matrix_width,
								matrix_height,
								preferred_local_size,
								host_cores,
								a_data_sizes,
								global_sizes);
							printf("(rows: "); 
							for(unsigned int i = 0; i < device_count; i++) {
								kernel_index[i] = 0;
								if(selected_devices[i].type == CL_DEVICE_TYPE_CPU)
									kernel_index[i] = 1;
								b_data_sizes[i].width = matrix_height;
								local_sizes[i].width = preferred_local_size.width;
								if(i == device_count - 1)
									printf("%d)...", a_data_sizes[i].height);
								else
									printf("%d : ", a_data_sizes[i].height);
							}
							
							/* Compute reference if needed */
							float* reference = (float*)malloc(matrix_height * matrix_height * sizeof(float));
							if(clProgramOptionAsBool(&verify_output)) {
								float* a = (float*)malloc(matrix_width * matrix_height * sizeof(float));						
								float* b = (float*)malloc(matrix_width * matrix_height * sizeof(float));
								computeInput(a, b, matrix_width, matrix_height, matrix_height, 0);
								computeOutput(a, b, reference, matrix_width, matrix_height, matrix_height);
								free(a);
								free(b);
							}
		
							SYSTEMTIME start, end;
							unsigned int samples;			
							pf_data.global_sizes = global_sizes;
							pf_data.local_sizes = local_sizes;
							pf_data.a_data_sizes = a_data_sizes;
							pf_data.b_data_sizes = b_data_sizes;
							pf_data.kernel_index = kernel_index;
							pf_data.type_size = 1;
							pf_data.src_flags = settings[memset_index][0].flags;
							pf_data.dst_flags = settings[memset_index][1].flags;
							pf_data.map_src = settings[memset_index][0].mapping;
							pf_data.map_dst = settings[memset_index][1].mapping;
							pf_data.reference = reference;

							CLProfilingResult result;
							if(device_count == 1) {
								result = clProfileComputation(
									runDeviceComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);

								//completion time more relevant for big matrices
								double completion_time = ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples);
								double weight = ((double)matrix_width * (double)matrix_height) / ((double)clProgramOptionAsInt(&max_matrix_size) * (double)clProgramOptionAsInt(&max_matrix_size));
								average_completion_time += pow(completion_time, weight);
								test_count++;
							}
							else 
								result = clProfileComputation(
									runHeteroSeparatedComputation, 
									&pf_data,
									clProgramOptionAsInt(&tries),
									clProgramOptionAsInt(&test_duration),
									CL_PROFILING_RESULT_AVERAGE,
									&start, &end, &samples);
							
							*output << matrix_width << " x " << matrix_height << ";" << samples << ";";
							*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
							*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
							*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples) << ";";
							*output << result.alloc_time << ";" << result.init_time << ";" << result.exec_time << ";" << result.read_time;

							printf("%*.*lf ms (float1, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
		
							unsigned int to_wait = rand() % clProgramOptionAsInt(&wait);		
							Sleep(to_wait);
				
							free(reference);

							*output << std::endl;
						}
					}
				}
				
				if(device_count == 1) {
					performance_scales[current_combination[0]].device = selected_devices[0].id;
					performance_scales[current_combination[0]].completion_time = 1.0 / (average_completion_time / (double)test_count);
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
	
	std::cout << "----- MatrixMult test end - press any key to exit -----" << std::endl;
	getchar();
}


