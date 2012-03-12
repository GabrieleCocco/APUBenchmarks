#include "computations.h"

void initVectorPair(float* x, float* y, unsigned int size) {	
	for(unsigned int i = 0; i < size; i++) {
		x[i] = float(i);
		y[i] = float(size  -1 - i);
	}
}

bool verify(float* first, float* second, unsigned int size) {
	for(unsigned int i = 0; i < size; i++) {
		if(first[i] != second[i])
			return false;
	}
	return true;
}

DWORD WINAPI runThreadComputation(LPVOID iValue) {
	int* params = (int*)iValue;
	float* x_pointer = (float*)params[0]; 
	float* y_pointer = (float*)params[1];
	unsigned int size = (unsigned int)params[2];
	float a = (float)params[3];
	unsigned int index = (unsigned int)params[4];

	unsigned int start = index * size;	
	for (unsigned int i = 0; i < size; i++) 
		y_pointer[start + i] = (a * x_pointer[start + i]) + y_pointer[start + i]; 

	return 0;
}

CLProfilingResult runHostComputation(void* data) {
	SaxpyProfilingData* profiling_data = (SaxpyProfilingData*)data;
	
	unsigned int size = profiling_data->data_sizes[0];
	unsigned int global_size = profiling_data->global_sizes[0];
	bool verify_output = profiling_data->verify_output;
	float* reference = profiling_data->reference;
	float a = profiling_data->a;
	cl_mem_flags src_flags = profiling_data->src_flags;
	cl_mem_flags dst_flags = profiling_data->dst_flags;
	int map_src = profiling_data->map_src;
	int map_dst = profiling_data->map_dst;
	
	float* x_pointer = NULL;
	float* y_pointer = NULL;
	CLProfilingResult result;
	
	x_pointer = (float*) malloc(size * sizeof(float));
	y_pointer = (float*) malloc(size * sizeof(float));	
	
	initVectorPair(x_pointer, y_pointer, size);
	
	if(global_size == 1) {
		for (unsigned int i = 0; i < size; i++) 
			y_pointer[i] = (a * x_pointer[i]) + y_pointer[i]; 
	}
	else {		
		HANDLE* thread_handles = (HANDLE*)malloc(global_size * sizeof(HANDLE)); 
		int** params = (int**)malloc(global_size * sizeof(int*));
		for(unsigned int i = 0; i < global_size; i++) {
			params[i] = (int*)malloc(5 * sizeof(int));
			params[i][0] = (int)x_pointer;
			params[i][1] = (int)y_pointer;
			params[i][2] = size / global_size;
			params[i][3] = (int)a;
			params[i][4] = i;
		
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
				DWORD dwError = GetLastError();
				std::cout<<"SCM:Error in Creating thread"<<dwError<<std::endl ;
			}
		}
		for(unsigned int i = 0; i < global_size; i++)
			WaitForSingleObject(thread_handles[i],INFINITE);
		
		/* Free thread resource */
		free(thread_handles);
		for(unsigned int i = 0; i < global_size; i++)
			free(params[i]);
		free(params);
	}
	
	/* Verify result */
	bool ok = true;
	if(verify_output)
		ok = verify(y_pointer, reference, size);
	if(!ok)
		std::cout << "Computation failed!" << std::endl;

	free(x_pointer);
	free(y_pointer);

	result.success = 1;
	return result;
}

CLProfilingResult runDeviceComputation(void* data) {
	CLProfilingResult result;
	result.success = true;

	SaxpyProfilingData* profiling_data = (SaxpyProfilingData*)data;
	SaxpyDeviceData device_data;
	device_data.index = 0;
	device_data.offset = 0;
	device_data.profiling_data = profiling_data;
	
	handleComputation(&device_data);
	return result;
}

DWORD WINAPI handleComputation(void* data) {
	SaxpyDeviceData* h_data = (SaxpyDeviceData*)data;
	float a = h_data->profiling_data->a;
	cl_mem_flags src_flags = h_data->profiling_data->src_flags;
	cl_mem_flags dst_flags = h_data->profiling_data->dst_flags;
	int map_src = h_data->profiling_data->map_src;
	int map_dst = h_data->profiling_data->map_dst;
	unsigned int local_size = h_data->profiling_data->local_sizes[h_data->index];
	unsigned int global_size = h_data->profiling_data->global_sizes[h_data->index];
	unsigned int data_size = h_data->profiling_data->data_sizes[h_data->index];
	unsigned int type_size = h_data->profiling_data->type_size;
	unsigned int whole_data_size = 0;
	for(unsigned int i = 0; i < h_data->profiling_data->device_count; i++)
		whole_data_size += h_data->profiling_data->data_sizes[i] * h_data->profiling_data->type_size;
	CLDeviceEnvironment environment = h_data->profiling_data->environments[h_data->index];

	float* x;
	float* y;
	cl_mem x_buffer = NULL;
	cl_mem y_buffer = NULL;
	CLProfilingResult result;
	
	unsigned int offset = h_data->offset;
	bool ok = true;

	cl_int err = 0;
	//alloc vectors
	if(!map_src) 
		x = (float*)malloc(data_size * type_size * sizeof(float));
	if(!map_dst) 
		y = (float*)malloc(data_size * type_size * sizeof(float));
	x_buffer = clCreateBuffer(environment.context, src_flags, data_size * type_size * sizeof(float), NULL, NULL);
	y_buffer = clCreateBuffer(environment.context, dst_flags, data_size * type_size * sizeof(float), NULL, NULL);
	
	//init data
	if(map_src) 
		x = (float*)clEnqueueMapBuffer(environment.queue, x_buffer, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * type_size * data_size, 0, NULL, NULL, &err);
	if(map_dst)
		y = (float*)clEnqueueMapBuffer(environment.queue, y_buffer, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * type_size * data_size, 0, NULL, NULL, &err);
	for(unsigned int j = 0; j <  data_size * type_size; j++) {
		x[j] = j + offset;
		y[j] = whole_data_size  - 1 - j - offset;
	}
	if(map_src)
		clEnqueueUnmapMemObject(environment.queue, x_buffer, x, 0, NULL, NULL);
	else
		clEnqueueWriteBuffer(environment.queue, x_buffer, CL_FALSE, 0, data_size * type_size * sizeof(float), x, 0, NULL, NULL);
	if(map_dst)
		clEnqueueUnmapMemObject(environment.queue, y_buffer, y, 0, NULL, NULL);
	else
		clEnqueueWriteBuffer(environment.queue, y_buffer, CL_FALSE, 0, data_size * type_size * sizeof(float), y, 0, NULL, NULL);
		
	//compute sizes per device 
	int temp = 0;
	cl_kernel kernel = environment.kernels[h_data->profiling_data->kernel_index[h_data->index]];
	err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&x_buffer);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&y_buffer);
	err |= clSetKernelArg(kernel, 2, sizeof(float), (void*)&a);
	err |= clSetKernelArg(kernel, 3, sizeof(unsigned int), (void*)&temp);
	err |= clSetKernelArg(kernel, 4, sizeof(unsigned int), (void*)&data_size);
	err |= clEnqueueNDRangeKernel(
		environment.queue, 
		kernel,
		1, NULL, &global_size, &local_size, 0, NULL, NULL);
	
	if(map_dst) 
		y = (float*)clEnqueueMapBuffer(environment.queue, y_buffer, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * data_size * type_size, 0, NULL, NULL, NULL);
	else 
		clEnqueueReadBuffer(environment.queue, y_buffer, CL_TRUE, 0, data_size * type_size * sizeof(float), y, 0, NULL, NULL);		
	
	if(h_data->profiling_data->verify_output) {
		ok &= verify(y, h_data->profiling_data->reference + offset, data_size * type_size);		
		if(!ok)
			std::cout << "Computation failed!" << std::endl;
	}

	/* Free resources */
	if(!map_src)
		free(x);
	clReleaseMemObject(x_buffer);

	if(map_dst) 
		clEnqueueUnmapMemObject(environment.queue, y_buffer, y, 0, NULL, NULL);
	else
		free(y);
	clReleaseMemObject(y_buffer);

	if(!ok) {
		result.success = 0;
		return -1;
	}
	return 0;
}

CLProfilingResult runHeteroSeparatedComputation(void* data) {
	SaxpyProfilingData* pf_data = (SaxpyProfilingData*)data;
	CLProfilingResult result;
	result.success = true;

	HANDLE* thread_handles = (HANDLE*)malloc(pf_data->device_count * sizeof(HANDLE)); 
	SaxpyDeviceData* params = (SaxpyDeviceData*)malloc(pf_data->device_count * sizeof(SaxpyDeviceData));

	unsigned int offset = 0;
	for(unsigned int i = 0; i < pf_data->device_count; i++) {
		params[i].profiling_data = pf_data;
		params[i].offset = offset;
		params[i].index = i;
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

	for(unsigned int i = 0; i < pf_data->device_count; i++)
		WaitForSingleObject(thread_handles[i], INFINITE);
		
	/* Free thread resource */
	free(thread_handles);
	free(params);

	return result;
}
