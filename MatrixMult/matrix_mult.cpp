#include "matrix_mult.h"
#include <time.h>
#include <stdlib.h>
#include <WinBase.h>

bool verifyOut(float* first, float* second, unsigned int size) {
	for (unsigned int i = 0; i < size; i++) {
		if (first[i] != second[i])
			return false;
	}
	return true;
}

int main(int argc, char* argv[])
{
	std::cout << "----- Saxpy test start -----" << std::endl;

	size_t min_matrix_size = (64);
	size_t max_matrix_size = (2 << 10);
	char* log_file = "MatrixMultiplication.csv";
	int iterations = 100;
	int block_size = 16;
	SYSTEMTIME startTime;
	SYSTEMTIME endTime;

	char buildOptions[256];
	memset(buildOptions, 0, 256);
	sprintf(
		buildOptions, "-D BLOCK_SIZE=%d",
		block_size);

	CLMemorySetting device_copy[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY", CL_MEM_READ_ONLY, 0, &device_copy[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY", CL_MEM_WRITE_ONLY, 0, &device_copy[1]);
	/*CLMemorySetting device_copyWithMap[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY", CL_MEM_READ_ONLY, 1, &device_copyWithMap[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY", CL_MEM_WRITE_ONLY, 1, &device_copyWithMap[1]);*/
	CLMemorySetting host_map[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR", CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 1, &host_map[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR", CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 1, &host_map[1]);
	/*CLMemorySetting host_alloc[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR", CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_alloc[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR", CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 1, &host_alloc[1]);
	*/
#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	CLMemorySetting* settings[4] = {
		device_copy,
		//device_copyWithMap,
		host_map//,
		//host_alloc
	};
	unsigned int memset_count = 2;
#else
	CLMemorySetting* settings[4] = {
		device_copy,
		//device_copyWithMap,
		host_map//,
		//host_alloc
	};
	unsigned int memset_count = 2;
#endif

	/* Discover devices */
	std::cout << std::endl << "- Tested devices listed below" << std::endl;
	unsigned int dcount;
	CLDeviceInfo* devices;
	clListDevices(&dcount, &devices);

	/* Open log file */
	std::cout << std::endl << "- Setting log file...";
	std::ofstream* output;
	output = new std::ofstream(log_file, std::ios::trunc);
	*output << "PLATFORM;DEVICE;READ MODE;WRITE MODE;INPUT FLAGS;OUTPUT FLAGS;SIZE;TIME;ITERATIONS" << std::endl;

	std::cout << "DONE!" << std::endl;

	for (unsigned int i = 0; i < dcount; i++)  {
		CLDeviceInfo devInfo = devices[i];

		// Creake device, program, kernel
		printf("  %-35s", "Creating context...");
		cl_context context = clCreateContext(0, 1, &devices[i].id, NULL, NULL, NULL);
		printf("DONE!\n");

		printf("  %-35s", "Creating command queues...");
		cl_command_queue queue = clCreateCommandQueue(context, devices[i].id, 0, NULL);
		printf("DONE!\n");

		printf("  %-35s", "Loading kernel file...");
		const char* kernel_src;
		unsigned int kernel_size;
		clLoadFile("matrix_mult.cl", &kernel_size, &kernel_src);
		printf("DONE!\n");

		printf("  %-35s", "Creating program with source...");
		cl_program program = clCreateProgramWithSource(context, 1, &kernel_src, &kernel_size, NULL);
		printf("DONE!\n");

		printf("  %-35s", "Building program...");
		cl_int status = clBuildProgram(program, 0, NULL, buildOptions, NULL, NULL);
		if (status != CL_SUCCESS) {
			char build_log[2048];
			memset(build_log, 0, 2048);
			clGetProgramBuildInfo(program, devices[i].id, CL_PROGRAM_BUILD_LOG, 2048, build_log, NULL);
			printf("\n\nBUILD LOG: \n %s\n\n", build_log);
		}
		printf("DONE!\n");

		cl_int err;
		std::ostringstream o;
		o << "  Creating kernel ";

		printf("  %-35s", o.str().c_str());
		cl_kernel kernel = devInfo.type == CL_DEVICE_TYPE_CPU ? clCreateKernel(program, "matrixMulBase", &err) : clCreateKernel(program, "matrixMul", &err);
		if (err == 0)
			printf("DONE!\n");
		else
			printf("FAIL! (%d)\n", err);
		free((char*)kernel_src);

		// Start test				
		std::cout << std::endl;
		for (unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {
			printf("  %-15s (%d) - %15s (%d)\n", settings[memset_index][0].name, (int)settings[memset_index][0].flags, settings[memset_index][1].name, (int)settings[memset_index][1].flags);

			char* read_mode = "EnqueueReadBuffer";
			char* write_mode = "EnqueueWriteBuffer";
			if (settings[memset_index][0].mapping > 0)
				read_mode = "MapBuffer";
			if (settings[memset_index][1].mapping > 0)
				write_mode = "MapBuffer";


			for (unsigned int matrix_width = min_matrix_size;
				matrix_width <= max_matrix_size;
				matrix_width += 64)
			{
				unsigned int matrix_height = matrix_width;
					printf("- Vector size %5d x %5d elements...", matrix_width, matrix_height);
					size_t whole_size = matrix_width * matrix_height * sizeof(float);

					unsigned int samples;
					cl_mem_flags src_flags = settings[memset_index][0].flags;
					cl_mem_flags dst_flags = settings[memset_index][1].flags;
					bool map_src = settings[memset_index][0].mapping;
					bool map_dst = settings[memset_index][1].mapping;

					// Setup computation
					float* a;
					float* b;
					float* c;
					cl_mem a_buffer = NULL;
					cl_mem b_buffer = NULL;
					cl_mem c_buffer = NULL;
					CLProfilingResult result;

					bool ok = true;

					cl_int err = 0;

					// Init vectors
					a = (float*)malloc(whole_size);
					b = (float*)malloc(whole_size);
					c = (float*)malloc(matrix_height * matrix_height * sizeof(float));
					computeInput(a, b, matrix_width, matrix_height, matrix_height, 0);
					//float* reference = (float*)malloc(matrix_height * matrix_height * sizeof(float));
					//computeOutput(a, b, reference, matrix_width, matrix_height, matrix_height);

					GetSystemTime(&startTime);
					for (int iter = 0; iter < iterations; iter++) {
						// Init buffers
						if (src_flags & CL_MEM_USE_HOST_PTR) {
							a_buffer = clCreateBuffer(context, src_flags, whole_size, a, &err);
							b_buffer = clCreateBuffer(context, src_flags, whole_size, b, &err);
						}
						else {
							a_buffer = clCreateBuffer(context, src_flags, whole_size, NULL, &err);
							b_buffer = clCreateBuffer(context, src_flags, whole_size, NULL, &err);
							if (settings[memset_index][0].mapping) {
								float* ta = (float*)clEnqueueMapBuffer(queue, a_buffer, CL_TRUE, CL_MAP_WRITE, 0, whole_size, 0, NULL, NULL, &err);
								float* tb = (float*)clEnqueueMapBuffer(queue, b_buffer, CL_TRUE, CL_MAP_WRITE, 0, whole_size, 0, NULL, NULL, &err);
								RtlMoveMemory(ta, a, whole_size);
								RtlMoveMemory(tb, b, whole_size);
								err &= clEnqueueUnmapMemObject(queue, a_buffer, ta, 0, NULL, NULL);
								err &= clEnqueueUnmapMemObject(queue, b_buffer, tb, 0, NULL, NULL);
							}
							else {
								err = clEnqueueWriteBuffer(queue, a_buffer, CL_FALSE, 0, whole_size, a, 0, NULL, NULL);
								err = clEnqueueWriteBuffer(queue, b_buffer, CL_FALSE, 0, whole_size, b, 0, NULL, NULL);
							}
						}
						if (dst_flags & CL_MEM_USE_HOST_PTR)
							c_buffer = clCreateBuffer(context, dst_flags, matrix_height * matrix_height * sizeof(float), c, &err);
						else {
							c_buffer = clCreateBuffer(context, src_flags, matrix_height * matrix_height * sizeof(float), NULL, &err);
						}

						//compute sizes per device 
						size_t localWorkSize[] = { block_size, block_size };
						if (devInfo.type == CL_DEVICE_TYPE_CPU) {
							localWorkSize[0] = 16;
							localWorkSize[1] = 16;
						}
						size_t globalWorkSize[] = { matrix_height, matrix_height };
						err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&a_buffer);
						err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&b_buffer);
						err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&c_buffer);
						err |= clSetKernelArg(kernel, 3, sizeof(int), (void*)&matrix_width);
						err |= clSetKernelArg(kernel, 4, sizeof(int), (void*)&matrix_height);
						err |= clSetKernelArg(kernel, 5, sizeof(int), (void*)&matrix_height);
						err |= clEnqueueNDRangeKernel(
							queue,
							kernel,
							2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
						
						// Read back buffer
						float* res = c;
						if (dst_flags & CL_MEM_USE_HOST_PTR) {
							clFinish(queue);
						}
						else {
							if (settings[memset_index][1].mapping) {
								res = (float*)clEnqueueMapBuffer(queue, c_buffer, CL_TRUE, CL_MAP_READ, 0, matrix_height * matrix_height * sizeof(float), 0, NULL, NULL, &err);
							}
							else
								err = clEnqueueReadBuffer(queue, c_buffer, CL_TRUE, 0, matrix_height * matrix_height * sizeof(float), c, 0, NULL, NULL);
						}

						// Verify
						/*
					for (unsigned int ind = 0; ind < matrix_height * matrix_height; ind++) {
							if (res[ind] != 2 * 3 * matrix_width)
								ok = false;
						}
						ok &= true;
						if (!ok)
							std::cout << "Computation failed!" << std::endl;
							*/

						if (settings[memset_index][1].mapping) {
							err &= clEnqueueUnmapMemObject(queue, c_buffer, res, 0, NULL, NULL);
						}
						/* Free resources */
						clReleaseMemObject(a_buffer);
						clReleaseMemObject(b_buffer);
						clReleaseMemObject(c_buffer);
					}
					GetSystemTime(&endTime);
					long millis = clSystemTimeToMillis(endTime) - clSystemTimeToMillis(startTime);
					double timePerIter = (double)millis / (double)iterations;

					free(a);
					free(b);
					free(c);

					*output << "0;" << i << " " << devices[i].name << ";" << read_mode << ";" << write_mode << ";" << settings[memset_index][0].name << ";" << settings[memset_index][1].name << ";" << (matrix_width * matrix_height) << ";" << timePerIter << ";" << iterations << std::endl;

					printf("%*.*lf ms (float1, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(endTime) - clSystemTimeToMillis(startTime)) / (double)iterations, iterations);

					unsigned int to_wait = rand() % 500;
					Sleep(to_wait);

					//free(reference);
				
			}
		}
		clReleaseCommandQueue(queue);
		clReleaseKernel(kernel);
		clReleaseProgram(program);
		clReleaseContext(context);
	}

	*output << std::endl;
	std::cout << std::endl << "- Clear environment, devices and options...";
	output->flush();
	output->close();
	delete output;

	for (unsigned int i = 0; i < memset_count; i++) {
		clFreeMemorySetting(settings[i]);
	}
	for (unsigned int i = 0; i < dcount; i++) {
		clFreeDeviceInfo(&devices[i]);
	}
	free(devices);

	std::cout << "DONE!" << std::endl;

	std::cout << "----- Saxpy test end - press any key to exit -----" << std::endl;
	getchar();
}


