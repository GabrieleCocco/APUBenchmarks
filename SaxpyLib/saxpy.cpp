#include "saxpy.h"

long clSystemTimeToMillis(SYSTEMTIME time) {
	long millis = 0;
	millis += (time.wHour * 60 * 60 * 1000);
	millis += (time.wMinute * 60 * 1000);
	millis += (time.wSecond * 1000);
	millis += time.wMilliseconds;

	return millis;
}

void scatterWork(
	CLDeviceInfo* devices, 
	unsigned int device_count, 
	unsigned int data_size,
	unsigned int* preferred_local_sizes,
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
		global_sizes[i] = clRoundUp(data_sizes[i], preferred_local_sizes[i]);
		local_sizes[i] = preferred_local_sizes[i];
	}
}

SAXPYLIB_API void run(SaxpyCallback callback) {
	result = 0;
	GetSystemTime(&start);
	for(unsigned int i = 0; i < samples; i++) {
		result |= computation((void*)&pf_data);
	}
	GetSystemTime(&end);
	finish(callback);
}

SAXPYLIB_API void setup(int argc, char* argv[], cl_command_queue** queues, unsigned int* queues_count)
{	
	std::cout << "----- Saxpy test start -----" << std::endl;

	/* Parse args */
	unsigned int curr_index = 0;
	vector_size = atoi(argv[curr_index]);
	curr_index++;
	samples = atoi(argv[curr_index]);
	curr_index++;
	use_float4 = atoi(argv[curr_index]);
	curr_index++;
	vector_multiplier = (float)atof(argv[curr_index]);
	curr_index++;
	verify_output = atoi(argv[curr_index]);
	curr_index++;
	if(strncmp(argv[curr_index], "TEST_MODE_HOST", strlen("TEST_MODE_HOST")) == 0)
		test_mode = TEST_MODE_HOST;
	if(strncmp(argv[curr_index], "TEST_MODE_DEVICE", strlen("TEST_MODE_DEVICE")) == 0)
		test_mode = TEST_MODE_DEVICE;
	curr_index++;
	host_threads = atoi(argv[curr_index]);
	curr_index++;
	device_count = atoi(argv[curr_index]);
	curr_index++;
	kernel_path = argv[curr_index];
	curr_index++;
	kernel_options = argv[curr_index];
	curr_index++;
	for(unsigned int device_index = 0; device_index < device_count; device_index++) {
		device_ids[device_index] = atoi(argv[curr_index + (device_index * 5)]);
		curr_index++;
		local_sizes[device_index] = atoi(argv[curr_index + (device_index * 5)]);
		curr_index++;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_HOST_MAP", strlen("MEMORY_MODE_HOST_MAP")) == 0)
			input_memory_modes[device_index] = MEMORY_MODE_HOST_MAP;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_DEVICE_COPY", strlen("MEMORY_MODE_DEVICE_COPY")) == 0)
			input_memory_modes[device_index] = MEMORY_MODE_DEVICE_COPY;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_PERSISTENT_MEM_MAP", strlen("MEMORY_MODE_PERSISTENT_MEM_MAP")) == 0)
			input_memory_modes[device_index] = MEMORY_MODE_PERSISTENT_MEM_MAP;
		curr_index++;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_HOST_MAP", strlen("MEMORY_MODE_HOST_MAP")) == 0)
			output_memory_modes[device_index] = MEMORY_MODE_HOST_MAP;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_DEVICE_COPY", strlen("MEMORY_MODE_DEVICE_COPY")) == 0)
			output_memory_modes[device_index] = MEMORY_MODE_DEVICE_COPY;
		if(strncmp(argv[curr_index + (device_index * 5)], "MEMORY_MODE_PERSISTENT_MEM_MAP", strlen("MEMORY_MODE_PERSISTENT_MEM_MAP")) == 0)
			output_memory_modes[device_index] = MEMORY_MODE_PERSISTENT_MEM_MAP;
		curr_index++;
		kernel_functions[device_index] = argv[curr_index + (device_index * 5)];
		curr_index++;
	}
	log_file = argv[curr_index];
	curr_index++;
	append_to_file = atoi(argv[curr_index]);

	/* Create memory settings */
	for(unsigned int i = 0; i < device_count; i++) {
		switch(input_memory_modes[i]) {
			case MEMORY_MODE_DEVICE_COPY:
				clCreateMemorySetting("Input device copy", CL_MEM_READ_ONLY, 0, &input_memory_settings[i]);
			case MEMORY_MODE_HOST_MAP:
				clCreateMemorySetting("Input host map", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &input_memory_settings[i]);
			case MEMORY_MODE_PERSISTENT_MEM_MAP:
				clCreateMemorySetting("Input persistent mem map", CL_MEM_READ_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &input_memory_settings[i]);
		}
		switch(output_memory_modes[i]) {
			case MEMORY_MODE_DEVICE_COPY:
				clCreateMemorySetting("Output device copy", CL_MEM_WRITE_ONLY, 0, &output_memory_settings[i]);
			case MEMORY_MODE_HOST_MAP:
				clCreateMemorySetting("Output host map", CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &output_memory_settings[i]);
			case MEMORY_MODE_PERSISTENT_MEM_MAP:
				clCreateMemorySetting("Output persistent mem map", CL_MEM_WRITE_ONLY | CL_MEM_USE_PERSISTENT_MEM_AMD, 1, &output_memory_settings[i]);
		}
	}
	
	/* Compute number of cores */	
	unsigned int host_cores = clCpuCoreCount();
	if(host_threads > 0)
		host_cores = host_threads;
	std::cout << std::endl << "- Compute number of CPU cores..." << clCpuCoreCount() << " (will use " << host_cores << ")" << std::endl;
	
	/* Discover devices */
	std::cout << std::endl << "- Tested devices listed below" << std::endl;
	clListDevices(&platform_device_count, &platform_devices);
	clPrintDeviceList(platform_devices, platform_device_count, "  ");
	
	/* Open log file */
	std::cout << std::endl << "- Setting log file...";
	if(append_to_file)
		output = new std::ofstream(log_file, std::ios::app);
	else
		output = new std::ofstream(log_file, std::ios::trunc);
	std::cout << "DONE!" << std::endl;

	/* Set up data struct required by computations */
	pf_data.a = vector_multiplier;
	pf_data.verify_output = verify_output;
		
	/* OpenCL for each OpenCL device */
	if(test_mode == TEST_MODE_DEVICE) {
		std::cout << std::endl << "- Testing following devices" << std::endl;
		for(unsigned int i = 0; i < device_count; i++) {
			char* type;
			clDeviceTypeToString(platform_devices[device_ids[i]].type, &type);
			std::cout << "    " << platform_devices[device_ids[i]].name << "[" << type << "]..." << std::endl;
			*output << platform_devices[device_ids[i]].name << "(" << type << ")";
			free(type);
		}
		std::cout << std::endl;
		*output << std::endl;

		/* Create opencl environment for each device */
		std::cout << "- Creating opencl environment...";
		selected_devices = (CLDeviceInfo*)malloc(device_count * sizeof(CLDeviceInfo));
		for(unsigned int i = 0; i < device_count; i++)
			selected_devices[i] = platform_devices[device_ids[i]];
		clCreateDeviceEnvironment(
			selected_devices,
			device_count,
			kernel_path,
			kernel_functions,
			device_count,
			kernel_options,
			false,
			false,
			pf_data.environments);
								
		pf_data.device_count = device_count;

		/* Set command queues created */
		*queues = (cl_command_queue*)malloc(device_count * sizeof(cl_command_queue));
		for(unsigned int i = 0; i < device_count; i++)
			queues[i] = &pf_data.environments[i].queue;
		*queues_count = device_count;
				
		std::cout << std::endl;
		for(unsigned int i = 0; i < device_count; i++) {
			printf("Device %s: %-15s - %15s\n", platform_devices[i].name, input_memory_settings[i].name, output_memory_settings[i].name);
			*output << "[" << platform_devices[i].name << ": " << input_memory_settings[i].name << "-" << output_memory_settings[i].name << "]" << std::endl;
		}
		*output << "Vector size (bytes);Samples;Start time;End time;Duration";
		
		printf("- Vector size %10d bytes...", vector_size);

		/* Set up sizes */
		if(use_float4) {			
			scatterWork(
				selected_devices, 
				device_count, 
				vector_size / sizeof(float) / 4, 
				local_sizes,
				host_cores,
				pf_data.data_sizes, pf_data.global_sizes, pf_data.local_sizes);
			for(unsigned int i = 0; i < device_count; i++) 
				pf_data.kernel_index[i] = i;
			pf_data.type_size = 4;
		}
		else {
			scatterWork(
				selected_devices, 
				device_count, 
				vector_size / sizeof(float), 
				local_sizes,
				host_cores,
				pf_data.data_sizes, pf_data.global_sizes, pf_data.local_sizes);

			for(unsigned int i = 0; i < device_count; i++) 
				pf_data.kernel_index[i] = i;
			pf_data.type_size = 1;
		}

		/* Compute reference if needed */
		pf_data.reference = (float*)malloc(vector_size);
		if(verify_output) {
			float* x = (float*)malloc(vector_size);
			float* y = (float*)malloc(vector_size);
			computeInput(x, y, vector_size / sizeof(float));
			computeOutput(x, y, pf_data.reference, vector_multiplier, vector_size / sizeof(float));
			free(x);
			free(y);
		}

		/* Set up memory flags and mapping */
		for(unsigned int i = 0; i < device_count; i++) {
			pf_data.src_flags[i] = input_memory_settings[i].flags;
			pf_data.dst_flags[i] = output_memory_settings[i].flags;
			pf_data.map_src[i] = input_memory_settings[i].mapping;
			pf_data.map_dst[i] =  output_memory_settings[i].mapping;
		}

		/* Setup computation args */	

		/* Set correct computation to be executed */
		if(device_count == 1)
			computation = runDeviceComputation;
		else
			computation = runHeteroSeparatedComputation;
	}
	else {
		std::cout << std::endl << "- Testing CPU (" << host_cores << " cores)..." << std::endl;
		*output << "CPU (" << host_cores << " cores)" << std::endl;
		*output << "Vector size (bytes);Samples;Start time;End time;Duration" << std::endl;
						
		printf("- Vector size %10d bytes...", vector_size);
				
		/* Set up sizes */
		unsigned int global_size = host_cores;
		unsigned int data_size = vector_size / sizeof(float);

		/* Compute reference if needed */
		pf_data.reference = (float*)malloc(vector_size);	
		if(verify_output) {
			float* x = (float*)malloc(vector_size);
			float* y = (float*)malloc(vector_size);
			computeInput(x, y, data_size);
			computeOutput(x, y, pf_data.reference, vector_multiplier, data_size);
			free(x);
			free(y);
		}
		
		pf_data.global_sizes[0] = global_size;
		pf_data.data_sizes[0] = data_size;

		*queues = NULL;
		*queues_count = 0;

		computation = runHostComputation;
	}
}

void finish(SaxpyCallback callback) {
	*output << vector_size << ";" << samples << ";";
	*output << start.wHour << ":" << start.wMinute << ":" << start.wSecond << ":" << start.wMilliseconds << ";";
	*output << end.wHour << ":" << end.wMinute << ":" << end.wSecond << ":" << end.wMilliseconds << ";"; 
	*output << ((double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples);

	printf("%*.*lf ms (float1, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(end) - clSystemTimeToMillis(start)) / (double)samples, samples);
				
	*output << std::endl;

	/* Free resources */
	std::cout << std::endl << "- Clear environment, devices and options...";
	output->flush();
	output->close();
	delete output;
	
	clFreeDeviceEnvironments(pf_data.environments, device_count, false);
	free(selected_devices);
	
	for(unsigned int i = 0; i < device_count; i++) {
		clFreeMemorySetting(&input_memory_settings[i]);
		clFreeMemorySetting(&output_memory_settings[i]);
	}
	for(unsigned int i = 0; i < platform_device_count; i++) {
		clFreeDeviceInfo(&platform_devices[i]);
	}
	free(platform_devices);

	std::cout << "DONE!" << std::endl;	
	std::cout << "----- Saxpy test end - press any key to exit -----" << std::endl;
	
	/* Invoke callback */
	callback(start, end, result);
}

