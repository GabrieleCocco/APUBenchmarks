#include "computation.h"

void computeInput(float* a, float* b, unsigned int width_a, unsigned int height_a, unsigned int width_b, unsigned int offset) {
	srand(time(NULL));

	unsigned int height_b = width_a;

	for(unsigned int i = 0; i < height_a; i++) {
		for(unsigned int j = 0; j < width_a; j++) 
			a[i * width_a + j] = 1;
	}
	for(unsigned int i = 0; i < height_b; i++) {
		for(unsigned int j = 0; j < width_b; j++) 
			b[i * width_b + j] = 1;
	}
}

void computeOutput(float* a, float* b, float* c, unsigned int width_a, unsigned int height_a, unsigned int width_b) {
	unsigned int height_b = width_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;

    for (unsigned int i = 0; i < height_a; ++i) {
        for (unsigned int j = 0; j < width_b; ++j) {
            double sum = 0;
            for (unsigned int k = 0; k < width_a; ++k) {
                sum += a[i * width_a + k] * b[k * width_b + j];
            }
            c[i * width_c + j] = (float)sum;
        }
	}
}

bool verifyOutput(float* test, float* reference, int width, int height, int reference_offset) {
	int i = 0;
	for(i = 0; i < height; i++) {
		for(int j = 0; j < width; j++) {
			//Approximate to the second decimal
			float diff = reference[(i + reference_offset) * width + j] - test[i * width + j];
			diff = diff < 0 ? -diff: diff;
			if(diff > 1) {
				std::cout << "DIFF in REF[" << (i + reference_offset) << "][" << j << "], TEST[" << i << "][" << j << "] : REF = " << reference[(i + reference_offset) * width + j] << ", TEST = " << test[i * width + j] << std::endl;
				return false;
			}
		}
	}
	return true;
}

DWORD WINAPI runThreadComputation(LPVOID iValue) {
	int* params = (int*)iValue;
	float* matA = (float*)params[0]; 
	float* matB = (float*)params[1];
	float* matC = (float*)params[2];
	unsigned int width_a = (unsigned int)params[3];
	unsigned int height_a = (unsigned int)params[4];
	unsigned int width_b = (unsigned int)params[5];
	unsigned int rows_per_thread = (unsigned int)params[6];
	unsigned int index = (unsigned int)params[7];
	
	unsigned int height_b = width_a;
	unsigned int width_c = width_b;
	unsigned int height_c = height_a;
	
	unsigned int start_row = index * rows_per_thread;

	for (unsigned int i = start_row; i < start_row + rows_per_thread; ++i) {
        for (unsigned int j = 0; j < width_b; ++j) {
            double sum = 0;
            for (unsigned int k = 0; k < width_a; ++k) {
                double a = matA[i * width_a + k];
                double b = matB[k * width_b + j];
                sum += a * b;
            }
            matC[i * width_c + j] = (float)sum;
        }
	}

	return 0;
}

CLProfilingResult runHostComputation(void* data) {
	MatrixMultProfilingData* pf_data = (MatrixMultProfilingData*)data;

	unsigned int width_a = pf_data->a_data_sizes[0].width;
	unsigned int height_a = pf_data->a_data_sizes[0].height;
	unsigned int width_b = pf_data->b_data_sizes[0].width;

	unsigned int threads = pf_data->global_sizes[0].height;
	bool verify_output = pf_data->verify_output;

	unsigned int height_b = width_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;
	
	Timer exec_timer, alloc_timer, init_timer, read_timer;
	float* matA = NULL;
	float* matB = NULL;
	float* matC = NULL;
	CLProfilingResult result;
	result.alloc_time = result.init_time = result.exec_time = result.read_time = 0;
	result.success = true;

	//alloc matrix
	alloc_timer.start();
	matA = (float*)malloc(width_a * height_a * sizeof(float));
	matB = (float*)malloc(width_b * height_b * sizeof(float));
	matC = (float*)malloc(width_c * height_c * sizeof(float));
	result.alloc_time = alloc_timer.get();

	//init matrix
	init_timer.start();
	computeInput(matA, matB, width_a, height_a, width_b, 0);
	result.init_time = init_timer.get();

	unsigned int thread_index = 0;
	int** params = NULL;
	HANDLE* thread_handles = NULL;

	if(threads == 1) {	
		//execute multiplication
		exec_timer.start();
		for (unsigned int i = 0; i < height_a; ++i) {
			for (unsigned int j = 0; j < width_b; ++j) {
				double sum = 0;
				for (unsigned int k = 0; k < width_a; ++k) {
					double a = matA[i * width_a + k];
					double b = matB[k * width_b + j];
					sum += a * b;
				}
				matC[i * width_c + j] = (float)sum;
			}
		}
		result.exec_time = exec_timer.get();
	}
	else {		
		unsigned int rows_per_thread = height_a / threads;

		exec_timer.start();
		thread_handles = (HANDLE*)malloc(threads * sizeof(HANDLE)); 
		params = (int**)malloc(threads * sizeof(int*));
		for(thread_index = 0; thread_index < threads; thread_index++) {
			params[thread_index] = (int*)malloc(8 * sizeof(int));
			params[thread_index][0] = (int)matA;
			params[thread_index][1] = (int)matB;
			params[thread_index][2] = (int)matC;
			params[thread_index][3] = width_a;
			params[thread_index][4] = height_a;
			params[thread_index][5] = width_b;
			params[thread_index][6] = rows_per_thread;
			params[thread_index][7] = thread_index;
		
			//Assume size = N * threads
			DWORD dwGenericThread;
			thread_handles[thread_index] = CreateThread(
				NULL,
				0,
				runThreadComputation,
				params[thread_index],
				0,
				&dwGenericThread);
			
			if(thread_handles[thread_index] == NULL) {
				result.success = false;
				goto free;
			}
		}
		for(unsigned int i = 0; i < threads; i++)
			WaitForSingleObject(thread_handles[i],INFINITE);
		result.exec_time = exec_timer.get();
	}

	/* Verify result */
	if(verify_output) 
		result.success = verifyOutput(matC, pf_data->reference, width_c, height_c, 0);

free:
	for(unsigned int i = 0; i < thread_index; i++) 
		free(params[i]);
	free(params);
	free(thread_handles);

	/* Free resources */
	free(matA);
	free(matB);
	free(matC);
	
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;
	return result;
}

DWORD WINAPI handleComputation(void* data) {
	cl_int error = CL_SUCCESS;

	MatrixMultDeviceData* device_data = (MatrixMultDeviceData*)data;
	MatrixMultProfilingData* pf_data = device_data->profiling_data;
	unsigned int device_index = device_data->index;
	
	cl_context context = pf_data->environments[device_index].context;
	cl_command_queue queue = pf_data->environments[device_index].queue;
	cl_kernel kernel = pf_data->environments[device_index].kernels[pf_data->kernel_index[device_index]];

	unsigned int width_a = pf_data->a_data_sizes[device_index].width;
	unsigned int height_a = pf_data->a_data_sizes[device_index].height;
	unsigned int width_b = pf_data->b_data_sizes[device_index].width;
	unsigned int block_size = pf_data->local_sizes[device_index].width;

	cl_mem_flags src_flags = pf_data->src_flags; 
	cl_mem_flags dst_flags = pf_data->dst_flags; 
	int map_src = pf_data->map_src;
	int map_dst = pf_data->map_dst;
	bool verify_output = pf_data->verify_output;
		
	unsigned int height_b = width_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;

	float* pA = NULL;
	float* pB = NULL;
	float* pC = NULL;
    cl_mem bufA;
    cl_mem bufC;
    cl_mem bufB;
	CLProfilingResult result;
	result.success = 1;

	//alloc vectors
	if(!map_src) {
		pA = (float*)malloc(width_a * height_a * sizeof(float));
		pB = (float*)malloc(width_b * height_b * sizeof(float));
	}
	bufA = clCreateBuffer(context, src_flags, width_a * height_a * sizeof(float), NULL, &error);
	bufB = clCreateBuffer(context, src_flags, height_b * width_b * sizeof(float), NULL, &error);
	bufC = clCreateBuffer(context, dst_flags, width_c * height_c * sizeof(float), NULL, &error);

	//init data
	if(map_src) {
		pA = (float*)clEnqueueMapBuffer(queue, bufA, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * width_a * height_a, 0, NULL, NULL, &error);
		pB = (float*)clEnqueueMapBuffer(queue, bufB, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * height_b * width_b, 0, NULL, NULL, &error);
	}
	computeInput(pA, pB, width_a, height_a, width_b, device_data->offset);
	if(map_src) {
		clEnqueueUnmapMemObject(queue, bufA, pA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(queue, bufB, pB, 0, NULL, NULL);
	}
	else {
		clEnqueueWriteBuffer(queue, bufA, CL_TRUE, 0, width_a * height_a * sizeof(float), pA, 0, NULL, NULL);
		clEnqueueWriteBuffer(queue, bufB, CL_TRUE, 0, height_b * width_b * sizeof(float), pB, 0, NULL, NULL);
	}
	
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &bufC);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &bufA);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &bufB);
    clSetKernelArg(kernel, 3, sizeof(cl_int), (void *) &width_a);
    clSetKernelArg(kernel, 4, sizeof(cl_int), (void *) &height_a);
    clSetKernelArg(kernel, 5, sizeof(cl_int), (void *) &width_b);
	
    size_t localWorkSize[] = { block_size, block_size };
    size_t globalWorkSize[] = { clRoundUp(width_c, block_size), clRoundUp(height_c, block_size) };
	// Multiplication - non-blocking execution:  launch and push to device(s)
	
	error = clEnqueueNDRangeKernel(queue, kernel, 2, 0, globalWorkSize, localWorkSize, 0, NULL, NULL);
	clFinish(queue);

	//read result
	if(map_dst) {
		pC = (float*)clEnqueueMapBuffer(queue, bufC, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_c * height_c, 0, NULL, NULL, &error);
		if(error != CL_SUCCESS)
			printf("ERR\n");
	}
	else {
		pC = (float*)malloc(width_c * height_c * sizeof(float));
		error = clEnqueueReadBuffer(queue, bufC, CL_TRUE, 0, width_c * height_c * sizeof(float), pC, 0, NULL, NULL);
		if(error != CL_SUCCESS)
			printf("ERR\n");
	}

	/* Verify result */
	if(verify_output) 
		result.success = verifyOutput(pC, pf_data->reference, width_c, height_c, device_data->offset);

	/* Free resources */
	if(map_src) {
		clEnqueueUnmapMemObject(queue, bufA, pA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(queue, bufB, pB, 0, NULL, NULL);
	}
	else {
		free(pA);
		free(pB);
	}
	clReleaseMemObject(bufA);
	clReleaseMemObject(bufB);

	if(map_dst) 
		clEnqueueUnmapMemObject(queue, bufC, pC, 0, NULL, NULL);
	else
		free(pC);
	clReleaseMemObject(bufC);
	
	if(!result.success) 
		std::cout << "Computation failed!" << std::endl;

	return 0;
}

CLProfilingResult runDeviceComputation(void* data) {
	CLProfilingResult result;
	result.success = true;

	MatrixMultProfilingData* profiling_data = (MatrixMultProfilingData*)data;
	MatrixMultDeviceData device_data;
	device_data.index = 0;
	device_data.offset = 0;
	device_data.profiling_data = profiling_data;
	
	handleComputation(&device_data);
	return result;
}

CLProfilingResult runHeteroSeparatedComputation(void* data) {
	MatrixMultProfilingData* pf_data = (MatrixMultProfilingData*)data;
	CLProfilingResult result;
	result.success = true;

	HANDLE* thread_handles = (HANDLE*)malloc(pf_data->device_count * sizeof(HANDLE)); 
	MatrixMultDeviceData* params = (MatrixMultDeviceData*)malloc(pf_data->device_count * sizeof(MatrixMultDeviceData));

	unsigned int offset = 0;
	for(unsigned int i = 0; i < pf_data->device_count; i++) {
		params[i].profiling_data = pf_data;
		params[i].offset = offset;
		params[i].index = i;
		offset += pf_data->a_data_sizes[i].height;
		
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
