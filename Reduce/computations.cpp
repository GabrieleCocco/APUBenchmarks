#include "computations.h"

DWORD WINAPI runThreadComputation(LPVOID iValue) {
	int* params = (int*)iValue;
	int* test_input = (int*)params[0]; 
	int* test_output = (int*)params[1]; 
	unsigned int size = (unsigned int)params[2];
	unsigned int index = (unsigned int)params[3];

	unsigned int start = index * size;
	int accum = 0;
	for (unsigned int i = 0; i < size; i++)
		accum += test_input[start + i];
	test_output[index] = accum;
	return 0;
}

CLProfilingResult runHostComputation(void* data) {
	ReduceProfilingData* pf_data = (ReduceProfilingData*)data;
	unsigned int data_size = pf_data->data_sizes[0];
	unsigned int cpu_threads = pf_data->global_sizes[0];
	bool verify_output = pf_data->verify_output;
	
	int* test_input = NULL;
	int test_output = 0;
	int* temp_output = NULL;
	CLProfilingResult result;
	result.success = true;

	test_input = (int*) malloc(data_size * sizeof(int));	
	if(cpu_threads > 1) 
		temp_output = (int*)malloc(cpu_threads * sizeof(int));
	computeInput(test_input, data_size, 0);

	if(cpu_threads == 1) {
		test_output = 0;
		for (unsigned int i = 0; i < data_size; i++)
			test_output += test_input[i];
	}
	else {		
		test_output = 0;
		HANDLE* thread_handles = (HANDLE*)malloc(cpu_threads * sizeof(HANDLE)); 
		int** params = (int**)malloc(cpu_threads * sizeof(int*));
		for(unsigned int i = 0; i < cpu_threads; i++) {
			params[i] = (int*)malloc(4 * sizeof(int));
			params[i][0] = (int)test_input;
			params[i][1] = (int)temp_output;
			params[i][2] = data_size / cpu_threads;
			params[i][3] = i;
		
			//Assume size = N * threads
			DWORD dwGenericThread;
			thread_handles[i] = CreateThread(
				NULL,
				0,
				runThreadComputation,
				params[i],
				0,
				&dwGenericThread);
			
			if(thread_handles[i] == NULL) {
				result.success = false;
				for(unsigned int j = 0; j < i; i++)
					free(params[j]);
				free(params);
				free(test_input);
				free(temp_output);
				return result;
			}
		}
		for(unsigned int i = 0; i < cpu_threads; i++)
			WaitForSingleObject(thread_handles[i],INFINITE);

		/* Final reduce stage */		
		for(unsigned int i = 0; i < cpu_threads; i++)
			test_output += temp_output[i];
		
		/* Free thread resource */
		free(thread_handles);
		for(unsigned int i = 0; i < cpu_threads; i++)
			free(params[i]);
		free(params);
	}

	/* Verify result */
	if(verify_output)
		result.success = (compare(test_output, pf_data->reference) == 0);
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;

	free(test_input);
	if(cpu_threads > 1) 
		free(temp_output);

	result.success = 1;
	return result;
}

DWORD WINAPI handleComputation(void* data) {
	//Timer timer;
	//timer.start();
	ReduceDeviceData* h_data = (ReduceDeviceData*)data;
	unsigned int device_index = h_data->index;
	unsigned int device_offset = h_data->offset;

	cl_mem_flags src_flags = h_data->profiling_data->src_flags;
	cl_mem_flags dst_flags = h_data->profiling_data->dst_flags;
	int map_src = h_data->profiling_data->map_src;
	int map_dst = h_data->profiling_data->map_dst;

	unsigned int local_size = h_data->profiling_data->local_sizes[device_index];
	unsigned int global_size = h_data->profiling_data->global_sizes[device_index];
	unsigned int data_size = h_data->profiling_data->data_sizes[device_index];
	unsigned int type_size = h_data->profiling_data->type_size;

	CLDeviceEnvironment environment = h_data->profiling_data->environments[device_index];
	cl_kernel kernel = environment.kernels[h_data->profiling_data->kernel_index[device_index]];
	bool cpu = (environment.info.type == CL_DEVICE_TYPE_CPU);
	unsigned int reduce_limit = h_data->profiling_data->reduce_limit;
	if(reduce_limit == 0)
		reduce_limit = 2;
	
	int* test_input_pointer = NULL;
	int* test_output_pointer = NULL;
	cl_mem test_input_buffer = NULL;
	cl_mem test_output_buffer = NULL;
	int test_output = 0;

	//global_size = local_size * 2;
	unsigned int output_data_size = global_size / local_size;

	//alloc vectors
	if(!map_src) 
		test_input_pointer = (int*)malloc(data_size * type_size * sizeof(int));
	if(!map_dst)
		test_output_pointer = (int*)malloc(output_data_size * type_size * sizeof(int));		
	test_input_buffer = clCreateBuffer(environment.context, src_flags, data_size * type_size * sizeof(int), NULL, NULL);
	test_output_buffer = clCreateBuffer(environment.context, dst_flags, output_data_size * type_size * sizeof(int), NULL, NULL);
	
	//init data
	if(map_src) 
		test_input_pointer = (int*)clEnqueueMapBuffer(environment.queue, test_input_buffer, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int) * type_size * data_size, 0, NULL, NULL, NULL);
	computeInput(test_input_pointer, data_size * type_size, device_offset);
	if(map_src)
		clEnqueueUnmapMemObject(environment.queue, test_input_buffer, test_input_pointer, 0, NULL, NULL);		
	else 
		clEnqueueWriteBuffer(environment.queue, test_input_buffer, CL_FALSE, 0, data_size * type_size * sizeof(int), test_input_pointer, 0, NULL, NULL);
		
	//The first iteration kernel write on test_output_buffer. 
	//The second read and write on it.
	//Ex: first: 2048 elements and 128 local size => 2048 / 128 elements = 16 elements
	//second 16 elements and 16 local_size => 1 element
	//set arguments

	//execute
	unsigned int ratio = 2;
	unsigned int new_output_data_size = output_data_size;
	clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&test_input_buffer);
	if(!cpu)
		clSetKernelArg(kernel, 1, local_size * type_size * sizeof(int), NULL);
	else {
		unsigned int block_size = data_size / global_size;
		clSetKernelArg(kernel, 1, sizeof(cl_int), &block_size);
	}
	clSetKernelArg(kernel, 2, sizeof(cl_int), &data_size);
	clSetKernelArg(kernel, 3, sizeof(cl_mem), (void*)&test_output_buffer);
	clEnqueueNDRangeKernel(environment.queue, kernel, 1, NULL, &global_size, &local_size, 0, NULL, NULL);

	//Update data_size = output of the previous iteration (output_data_size), global_size proportionally changed
	if(!cpu) {
		while(new_output_data_size >= reduce_limit) {
			unsigned int new_global_size = new_output_data_size / ratio;
			unsigned int new_data_size = new_output_data_size;
			unsigned int new_local_size = local_size > new_global_size ? new_global_size : local_size;
			new_output_data_size = new_global_size / new_local_size;
			clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&test_output_buffer);
			clSetKernelArg(kernel, 1, new_local_size * type_size * sizeof(int), NULL);
			clSetKernelArg(kernel, 2, sizeof(cl_int), &new_data_size);
			clEnqueueNDRangeKernel(environment.queue, kernel, 1, NULL, &new_global_size, &new_local_size, 0, NULL, NULL);
		}
	}

	//read result
	if(map_dst) 
		test_output_pointer = (int*)clEnqueueMapBuffer(environment.queue, test_output_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(int) * new_output_data_size * type_size, 0, NULL, NULL, NULL);
	else	
		clEnqueueReadBuffer(environment.queue, test_output_buffer, CL_TRUE, 0, new_output_data_size * type_size * sizeof(int), test_output_pointer, 0, NULL, NULL);		
	for(unsigned int i = 0; i < new_output_data_size * type_size; i++)
		test_output += test_output_pointer[i];
	
	//set result
	*h_data->output = test_output;

	/* Free resources */
	if(!map_src)
		free(test_input_pointer);
	clReleaseMemObject(test_input_buffer);

	if(map_dst) 
		clEnqueueUnmapMemObject(environment.queue, test_output_buffer, test_output_pointer, 0, NULL, NULL);
	else
		free(test_output_pointer);
	clReleaseMemObject(test_output_buffer);
	//*h_data->timer = timer.get();
	return 0;
}

CLProfilingResult runDeviceComputation(void* data) {
	CLProfilingResult result;
	result.success = true;
	
	int output = 0;
	//double timer = 0;
	ReduceProfilingData* pf_data = (ReduceProfilingData*)data;
	ReduceDeviceData device_data;
	device_data.index = 0;
	device_data.offset = 0;
	device_data.profiling_data = pf_data;
	device_data.output = &output;
	//device_data.timer = &timer;
	handleComputation(&device_data);
	
		//printf("\n[Thread %d time = %f]", 0, timer);
	/* Verify result */
	if(pf_data->verify_output)
		result.success = (compare(output, pf_data->reference) == 0);
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;

	return result;
}

CLProfilingResult runHeteroSeparatedComputation(void* data) {	
	CLProfilingResult result;
	result.success = true;
	
	ReduceProfilingData* pf_data = (ReduceProfilingData*)data;
	HANDLE* thread_handles = (HANDLE*)malloc(pf_data->device_count * sizeof(HANDLE)); 
	ReduceDeviceData* params = (ReduceDeviceData*)malloc(pf_data->device_count * sizeof(ReduceDeviceData));
	int* output = (int*)malloc(pf_data->device_count * sizeof(int));
	//double* timer = (double*)malloc(pf_data->device_count * sizeof(double));
	unsigned int offset = 0;
	#pragma unroll 2
	for(unsigned int i = 0; i < pf_data->device_count - 1; i++) {
		params[i].profiling_data = pf_data;
		params[i].offset = offset;
		params[i].index = i;
		params[i].output = &output[i];
		//params[i].timer = &timer[i];
		offset += pf_data->type_size * pf_data->data_sizes[i];
		
		//Assume size = N * threads
		DWORD dwGenericThread;
		thread_handles[i] = CreateThread(
			NULL,
			0,
			handleComputation,
			&params[i],
			0,
			&dwGenericThread);
			
		if(thread_handles[i] == NULL) {
			DWORD dwError = GetLastError();
			std::cout<<"SCM:Error in Creating thread"<<dwError<<std::endl ;
		}
	}
	
	#pragma unroll 2
	for(unsigned int i = 0; i < pf_data->device_count- 1; i++)
		WaitForSingleObject(thread_handles[i], INFINITE);
	/*
	for(unsigned int i = 0; i < pf_data->device_count; i++) {
		printf("\n[Thread %d time = %f (data = %d)]", i, timer[i], pf_data->data_sizes[i]);
	}
	printf("\n"); */
	/* Inter-device final reduce stage */
	#pragma unroll 2
	for(unsigned int i = 1; i < pf_data->device_count- 1; i++)
		output[0] += output[i];

	/* Verify result */
	if(pf_data->verify_output)
		result.success = (compare(output[0], pf_data->reference) == 0);
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;
	
	/* Free thread resource */
	free(thread_handles);
	free(params);
	free(output);
	//free(timer);

	return result;
}
