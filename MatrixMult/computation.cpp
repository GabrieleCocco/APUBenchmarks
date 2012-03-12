#include "computation.h"

void computeInput(float* pointer, unsigned int width, unsigned int height) {
	for(unsigned int i = 0; i < height; i++) {
		for(unsigned int j = 0; j < width; j++) 
			pointer[i * width + j] = (float)j / 100;
	}
}

float* computeOutput(float* matA, float* matB, unsigned int width_a, unsigned int height_a) {
	unsigned int height_b = width_a;
	unsigned int width_b = height_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;

	float* matC = (float*) malloc(width_c * height_c * sizeof(float));
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
	return matC;
}

bool verify(float* a, float* b, int size) {
	printf("Verifying...\n");
	int i = 0;
	int err = 0;
	for(i = 0; i < size && !err; i++) {
		//Approximate to the second decimal
		float diff = b[i] - a[i];
		diff = diff < 0 ? -diff: diff;
		if(diff > 0.1) {
			err = 1;
			std::cout << "DIFFERENCE " << i << ": A = " << a[i] << ", B = " << b[i] << std::endl;
			return false;
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

	CLEnvironment* env = pf_data->env;
	cl_kernel kernel = pf_data->kernel;
	unsigned int width_a = pf_data->width_a;
	unsigned int height_a = pf_data->height_a;
	unsigned int threads = pf_data->cpu_threads;
	cl_mem_flags src_flags = pf_data->src_flags; 
	cl_mem_flags dst_flags = pf_data->dst_flags; 
	int map_src = pf_data->map_src;
	int map_dst = pf_data->map_dst;
	bool verify_output = pf_data->verify_output;

	unsigned int height_b = width_a;
	unsigned int width_b = height_a;
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
	computeInput(matA, width_a, height_a);
	computeInput(matB, width_b, height_b);
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
	if(verify_output) {
		float* ref = computeOutput(matA, matB, width_a, height_a);
		result.success = verify(matC, ref, width_c * height_c);
		free(ref);
	}

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

CLProfilingResult runHostGpuComputation(void* data) {
	MatrixMultProfilingData* pf_data = (MatrixMultProfilingData*)data;

	CLEnvironment* env = pf_data->env;
	cl_kernel kernel = pf_data->kernel;
	unsigned int width_a = pf_data->width_a;
	unsigned int height_a = pf_data->height_a;
	unsigned int threads = pf_data->cpu_threads;
	unsigned int block_size = pf_data->block_size;
	cl_mem_flags src_flags = pf_data->src_flags; 
	cl_mem_flags dst_flags = pf_data->dst_flags; 
	int map_src = pf_data->map_src;
	int map_dst = pf_data->map_dst;
	bool verify_output = pf_data->verify_output;
	
	unsigned int height_b = width_a;
	unsigned int width_b = height_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;
	 
	unsigned int height_a_per_device = height_a / 2;
	unsigned int height_c_per_device = height_c / 2;

	Timer exec_timer, alloc_timer, init_timer, read_timer;	
	// matA and matB shared between CPU threads and GPU (cause read-only)
	float* matA = NULL;
	float* matB = NULL;
	float* matC_CPU = NULL;
	float* matC_GPU = NULL;
    cl_mem bufA;
    cl_mem bufC;
    cl_mem bufB;
	CLProfilingResult result;
	result.alloc_time = result.init_time = result.exec_time = result.read_time = 0;
	result.success = 1;

	// GPU part 
	//alloc vectors
	if(!map_src) {
		alloc_timer.start();
		matA = (float*)malloc(width_a * height_a * sizeof(float));
		matB = (float*)malloc(height_b * width_b * sizeof(float));
		bufA = clCreateBuffer(env->context, src_flags, width_a * height_a_per_device * sizeof(float), NULL, NULL);
		bufB = clCreateBuffer(env->context, src_flags, height_b * width_b * sizeof(float), NULL, NULL);
		//Only the part written by the GPU
		bufC = clCreateBuffer(env->context, dst_flags, height_c * height_c_per_device * sizeof(float), NULL, NULL);
		result.alloc_time = alloc_timer.get();
	}
	else {
		alloc_timer.start();
		bufA = clCreateBuffer(env->context, src_flags, width_a * height_a * sizeof(float), NULL, NULL);
		bufB = clCreateBuffer(env->context, src_flags, height_b * width_b * sizeof(float), NULL, NULL);
		//Only the part written by the GPU
		bufC = clCreateBuffer(env->context, dst_flags, width_c * height_c_per_device * sizeof(float), NULL, NULL);
		result.alloc_time = alloc_timer.get();
	}

	//init data
	if(map_src) {
		init_timer.start();
		matA = (float*)clEnqueueMapBuffer(env->queue, bufA, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * width_a * height_a, 0, NULL, NULL, NULL);
		matB = (float*)clEnqueueMapBuffer(env->queue, bufB, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * width_b * height_b, 0, NULL, NULL, NULL);
		
		computeInput(matA, width_a, height_a);
		computeInput(matB, width_b, height_b);
		//Unmap to actuate writes
		clEnqueueUnmapMemObject(env->queue, bufA, matA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(env->queue, bufB, matB, 0, NULL, NULL);
		//Remap for CPU usage
		matA = (float*)clEnqueueMapBuffer(env->queue, bufA, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_a * height_a, 0, NULL, NULL, NULL);
		matB = (float*)clEnqueueMapBuffer(env->queue, bufB, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_b * height_b, 0, NULL, NULL, NULL);
	
		result.init_time = init_timer.get();
	}
	else {
		init_timer.start();
		computeInput(matA, width_a, height_a);
		computeInput(matB, width_b, height_b);
		//In case of transfer, trasfer only the half of A for the GPU
		clEnqueueWriteBuffer(env->queue, bufA, CL_TRUE, 0, width_a * height_a_per_device * sizeof(float), matA, 0, NULL, NULL);
		clEnqueueWriteBuffer(env->queue, bufB, CL_TRUE, 0, height_b * width_b * sizeof(float), matB, 0, NULL, NULL);
		result.init_time = init_timer.get();
	}
	
	//GPU part
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &bufC);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &bufA);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &bufB);
    clSetKernelArg(kernel, 3, sizeof(cl_int), (void *) &width_a);
	//Tell the GPU half the size of A
    clSetKernelArg(kernel, 4, sizeof(cl_int), (void *) &width_b);
	
    size_t localWorkSize[] = {block_size, block_size};
    size_t globalWorkSize[] = {width_c, height_c_per_device};
	

	//CPU part
	alloc_timer.start();
	matC_CPU = (float*)malloc(width_c * height_c_per_device * sizeof(float));
	result.alloc_time += alloc_timer.get();
	
	unsigned int rows_per_thread = height_a_per_device / threads;
	unsigned int thread_index = 0;
	HANDLE* thread_handles = NULL;
	int** params = NULL;

	exec_timer.start();
	thread_handles = (HANDLE*)malloc(threads * sizeof(HANDLE)); 
	params = (int**)malloc(threads * sizeof(int*));
	for(thread_index = 0; thread_index < threads; thread_index++) {
		params[thread_index] = (int*)malloc(8 * sizeof(int));
		params[thread_index][0] = (int)matA;
		params[thread_index][1] = (int)matB;
		params[thread_index][2] = (int)matC_CPU;
		params[thread_index][3] = width_a;
		params[thread_index][4] = height_a_per_device;
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
			result.success = 0;
			goto free;
		}
	}

	cl_int err = clEnqueueNDRangeKernel(env->queue, kernel, 2, 0, globalWorkSize, localWorkSize, 0, NULL, NULL);
		
	//read result
	if(map_dst) {
		read_timer.start();
		matC_GPU = (float*)clEnqueueMapBuffer(env->queue, bufC, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_c * height_c_per_device, 0, NULL, NULL, &err);
		if(err)
			printf("ERR\n");
		result.read_time = read_timer.get();
	}
	else {
		read_timer.start();
		matC_GPU = (float*)malloc(width_c * height_c_per_device * sizeof(float));
		err = clEnqueueReadBuffer(env->queue, bufC, CL_TRUE, 0, width_c * height_c_per_device * sizeof(float), matC_GPU, 0, NULL, NULL);
		if(err)
			printf("ERR\n");
		result.read_time = read_timer.get();
	}
	
	//clFinish(env->queue);
	for(unsigned int i = 0; i < threads; i++)
		WaitForSingleObject(thread_handles[i],INFINITE);

	result.exec_time = exec_timer.get();

	// Verify result 
	if(verify_output) {	
		float* ref = computeOutput(matA, matB, width_a, height_a);
		bool ok_cpu = verify(matC_CPU, ref + (width_c * height_c_per_device), width_c * height_c_per_device);
		bool ok_gpu = verify(matC_GPU, ref, width_c * height_c_per_device);
		result.success = ok_cpu & ok_gpu;
		free(ref);
	}
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;
	
free:
	for(unsigned int i = 0; i < thread_index; i++) 
		free(params[i]);
	free(params);
	free(thread_handles);
	free(matC_CPU);
	
	// Free resources 
	if(map_src) {
		clEnqueueUnmapMemObject(env->queue, bufA, matA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(env->queue, bufB, matB, 0, NULL, NULL);
	}	
	else {
		free(matA);
		free(matB);
	}
	clReleaseMemObject(bufA);
	clReleaseMemObject(bufB);

	if(map_dst) 
		clEnqueueUnmapMemObject(env->queue, bufC, matC_GPU, 0, NULL, NULL);
	else
		free(matC_GPU);
	clReleaseMemObject(bufC);
	
	if(!result.success)
		std::cout << "Computation failed!" << std::endl;
	return result;
}

CLProfilingResult runGpuComputation(void* data) {
	MatrixMultProfilingData* pf_data = (MatrixMultProfilingData*)data;

	CLEnvironment* env = pf_data->env;
	cl_kernel kernel = pf_data->kernel;
	unsigned int width_a = pf_data->width_a;
	unsigned int height_a = pf_data->height_a;
	unsigned int threads = pf_data->cpu_threads;
	unsigned int block_size = pf_data->block_size;
	cl_mem_flags src_flags = pf_data->src_flags; 
	cl_mem_flags dst_flags = pf_data->dst_flags; 
	int map_src = pf_data->map_src;
	int map_dst = pf_data->map_dst;
	bool verify_output = pf_data->verify_output;
		
	unsigned int height_b = width_a;
	unsigned int width_b = height_a;
	unsigned int height_c = height_a;
	unsigned int width_c = width_b;

	Timer exec_timer, alloc_timer, init_timer, read_timer;
	float* pA = NULL;
	float* pB = NULL;
	float* pC = NULL;
    cl_mem bufA;
    cl_mem bufC;
    cl_mem bufB;
	CLProfilingResult result;
	result.alloc_time = result.init_time = result.exec_time = result.read_time = 0;
	result.success = 1;
	
	cl_int err;
	//alloc vectors
	if(!map_src) {
		alloc_timer.start();
		pA = (float*)malloc(width_a * height_a * sizeof(float));
		pB = (float*)malloc(height_a * width_a * sizeof(float));
		bufA = clCreateBuffer(env->context, src_flags, width_a * height_a * sizeof(float), NULL, &err);
		bufB = clCreateBuffer(env->context, src_flags, height_b * width_b * sizeof(float), NULL, &err);
		bufC = clCreateBuffer(env->context, dst_flags, height_c * height_c * sizeof(float), NULL, &err);
		result.alloc_time = alloc_timer.get();
	}
	else {
		alloc_timer.start();
		bufA = clCreateBuffer(env->context, src_flags, width_a * height_a * sizeof(float), NULL, &err);
		bufB = clCreateBuffer(env->context, src_flags, height_b * width_b * sizeof(float), NULL, &err);
		bufC = clCreateBuffer(env->context, dst_flags, height_c * height_c * sizeof(float), NULL, &err);
		result.alloc_time = alloc_timer.get();
	}

	//init data
	if(map_src) {
		init_timer.start();
		pA = (float*)clEnqueueMapBuffer(env->queue, bufA, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * width_a * height_a, 0, NULL, NULL, &err);
		pB = (float*)clEnqueueMapBuffer(env->queue, bufB, CL_TRUE, CL_MAP_WRITE, 0, sizeof(float) * height_b * width_b, 0, NULL, NULL, &err);
		computeInput(pA, width_a, height_a);
		computeInput(pB, width_b, height_b);
		clEnqueueUnmapMemObject(env->queue, bufA, pA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(env->queue, bufB, pB, 0, NULL, NULL);
		result.init_time = init_timer.get();
	}
	else {
		init_timer.start();
		computeInput(pA, width_a, height_a);
		computeInput(pB, width_b, height_b);
		clEnqueueWriteBuffer(env->queue, bufA, CL_TRUE, 0, width_a * height_a * sizeof(float), pA, 0, NULL, NULL);
		clEnqueueWriteBuffer(env->queue, bufB, CL_TRUE, 0, height_b * width_b * sizeof(float), pB, 0, NULL, NULL);
		result.init_time = init_timer.get();
	}
	
    clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &bufC);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &bufA);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &bufB);
    clSetKernelArg(kernel, 3, sizeof(cl_int), (void *) &width_a);
    clSetKernelArg(kernel, 4, sizeof(cl_int), (void *) &width_b);
	
    size_t localWorkSize[] = {block_size, block_size};
    size_t globalWorkSize[] = {width_c, height_c};
	// Multiplication - non-blocking execution:  launch and push to device(s)
	
	exec_timer.start();
	err = clEnqueueNDRangeKernel(env->queue, kernel, 2, 0, globalWorkSize, localWorkSize, 0, NULL, NULL);
	clFinish(env->queue);
	result.exec_time = exec_timer.get();

	//read result
	if(map_dst) {
		cl_int err;
		read_timer.start();
		pC = (float*)clEnqueueMapBuffer(env->queue, bufC, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_c * height_c, 0, NULL, NULL, &err);
		if(err)
			printf("ERR\n");
		result.read_time = read_timer.get();
	}
	else {
		cl_int err;
		read_timer.start();
		pC = (float*)malloc(height_a * height_a * sizeof(float));
		err = clEnqueueReadBuffer(env->queue, bufC, CL_TRUE, 0, width_c * height_c * sizeof(float), pC, 0, NULL, NULL);
		if(err)
			printf("ERR\n");
		result.read_time = read_timer.get();
	}

	/* Verify result */
	if(verify_output) { 
		if(map_src) {
			pA = (float*)clEnqueueMapBuffer(env->queue, bufA, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * width_a * height_a, 0, NULL, NULL, &err);
			pB = (float*)clEnqueueMapBuffer(env->queue, bufB, CL_TRUE, CL_MAP_READ, 0, sizeof(float) * height_b * width_b, 0, NULL, NULL, &err);
		}
		float* ref = computeOutput(pA, pB, width_a, height_a);
		if(map_src) {
			clEnqueueUnmapMemObject(env->queue, bufA, pA, 0, NULL, NULL);
			clEnqueueUnmapMemObject(env->queue, bufB, pB, 0, NULL, NULL);
		}
		result.success = verify(pC, ref, width_c * height_c);
		free(ref);
	}

	/* Free resources */
	if(map_src) {
		clEnqueueUnmapMemObject(env->queue, bufA, pA, 0, NULL, NULL);
		clEnqueueUnmapMemObject(env->queue, bufB, pB, 0, NULL, NULL);
	}
	else {
		free(pA);
		free(pB);
	}
	clReleaseMemObject(bufA);
	clReleaseMemObject(bufB);

	if(map_dst) 
		clEnqueueUnmapMemObject(env->queue, bufC, pC, 0, NULL, NULL);
	else
		free(pC);
	clReleaseMemObject(bufC);
	
	if(!result.success) 
		std::cout << "Computation failed!" << std::endl;
	return result;
}