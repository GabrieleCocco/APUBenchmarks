#include "saxpy.h"
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

	size_t min_vector_size = 2048 * sizeof(float);
	size_t max_vector_size = (16 << 20) * sizeof(float);
	char* log_file = "VectorAdd.csv";
	int iterations = 100;
	SYSTEMTIME startTime;
	SYSTEMTIME endTime;

	CLMemorySetting device_copy[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY", CL_MEM_READ_ONLY, 0, &device_copy[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY", CL_MEM_WRITE_ONLY, 0, &device_copy[1]);
	CLMemorySetting host_map[2];
	clCreateMemorySetting("CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR", CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, 0, &host_map[0]);
	clCreateMemorySetting("CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR", CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, 0, &host_map[1]);
	
#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	CLMemorySetting* settings[3] = {
		device_copy,
		//device_copyWithMap,
		host_map };
	unsigned int memset_count = 2;
#else
	CLMemorySetting* settings[3] = {
		device_copy,
		//device_copyWithMap,
		host_map };
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
		clLoadFile("saxpy.cl", &kernel_size, &kernel_src);
		printf("DONE!\n");

		printf("  %-35s", "Creating program with source...");
		cl_program program = clCreateProgramWithSource(context, 1, &kernel_src, &kernel_size, NULL);
		printf("DONE!\n");

		printf("  %-35s", "Building program...");
		cl_int status = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
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
		cl_kernel kernel = clCreateKernel(program, "saxpy", &err);
		if (err == 0)
			printf("DONE!\n");
		else
			printf("FAIL! (%d)\n", err);		
		free((char*)kernel_src);

		// Start test				
		std::cout << std::endl;
		for(unsigned int memset_index = 0; memset_index < memset_count; memset_index++) {						
			printf("  %-15s - %15s\n", settings[memset_index][0].name, settings[memset_index][1].name);

			char* read_mode = "EnqueueReadBuffer";
			char* write_mode = "EnqueueWriteBuffer";
			if (settings[memset_index][0].mapping > 0)
				read_mode = "MapBuffer";
			if (settings[memset_index][1].mapping > 0)
				write_mode = "MapBuffer";
				
			for(unsigned int vector_size = min_vector_size; 
				vector_size <= max_vector_size;
				vector_size *= 2) 
			{
				printf("- Vector size %10d bytes...", vector_size);

				float* reference = (float*)malloc(vector_size);				
				float* x = (float*)malloc(vector_size);
				float* y = (float*)malloc(vector_size);
				computeInput(x, y, vector_size / sizeof(float));
				computeOutput(x, y, reference, vector_size / sizeof(float));
				free(x);
				free(y);

				unsigned int samples;
				cl_mem_flags src_flags = settings[memset_index][0].flags;
				cl_mem_flags dst_flags = settings[memset_index][1].flags;
				bool map_src = settings[memset_index][0].mapping;
				bool map_dst = settings[memset_index][1].mapping;
				unsigned int data_size = vector_size / sizeof(float);

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
				a = (float*)malloc(vector_size);
				b = (float*)malloc(vector_size);
				c = (float*)malloc(vector_size);
				for (unsigned int j = 0; j < data_size; j++) {
					a[j] = j;
					b[j] = data_size - j;
				}

				GetSystemTime(&startTime);
				for (int iter = 0; iter < iterations; iter++) {
					// Init buffers
					if (src_flags & CL_MEM_USE_HOST_PTR) {
						a_buffer = clCreateBuffer(context, src_flags, vector_size, a, &err);
						b_buffer = clCreateBuffer(context, src_flags, vector_size, b, &err);
					}
					else {
						a_buffer = clCreateBuffer(context, src_flags, vector_size, NULL, &err);
						b_buffer = clCreateBuffer(context, src_flags, vector_size, NULL, &err);
						if (settings[memset_index][0].mapping) {
							float* ta = (float*)clEnqueueMapBuffer(queue, a_buffer, CL_TRUE, CL_MAP_WRITE, 0, vector_size, 0, NULL, NULL, &err);
							float* tb = (float*)clEnqueueMapBuffer(queue, b_buffer, CL_TRUE, CL_MAP_WRITE, 0, vector_size, 0, NULL, NULL, &err);
							RtlMoveMemory(ta, a, vector_size);
							RtlMoveMemory(tb, b, vector_size);
							err &= clEnqueueUnmapMemObject(queue, a_buffer, ta, 0, NULL, NULL);
							err &= clEnqueueUnmapMemObject(queue, b_buffer, tb, 0, NULL, NULL);
						}
						else {
							err = clEnqueueWriteBuffer(queue, a_buffer, CL_FALSE, 0, vector_size, a, 0, NULL, NULL);
							err = clEnqueueWriteBuffer(queue, b_buffer, CL_FALSE, 0, vector_size, b, 0, NULL, NULL);
						}
					}
					if (dst_flags & CL_MEM_USE_HOST_PTR)
						c_buffer = clCreateBuffer(context, dst_flags, vector_size, c, &err);
					else {
						c_buffer = clCreateBuffer(context, src_flags, vector_size, NULL, &err);
					}

					//compute sizes per device 
					size_t local_size = 128;
					err |= clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&a_buffer);
					err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&b_buffer);
					err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&c_buffer);
					err |= clEnqueueNDRangeKernel(
						queue,
						kernel,
						1, NULL, &data_size, &local_size, 0, NULL, NULL);

					// Read back buffer
					float* res = c;
					if (dst_flags & CL_MEM_USE_HOST_PTR) {			
						clFinish(queue);
					}
					else {
						if (settings[memset_index][1].mapping) {
							res = (float*)clEnqueueMapBuffer(queue, c_buffer, CL_TRUE, CL_MAP_READ, 0, vector_size, 0, NULL, NULL, &err);
						}
						else
							err = clEnqueueReadBuffer(queue, c_buffer, CL_TRUE, 0, vector_size, c, 0, NULL, NULL);
					}

					// Verify
					/*
					for (unsigned int ind = 0; ind < data_size; ind++) {
						if (res[ind] != reference[ind])
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

				*output << "0;" << i << " " << devices[i].name << ";" << read_mode << ";" << write_mode << ";" << settings[memset_index][0].name << ";" << settings[memset_index][1].name << ";" << data_size << ";" << timePerIter << ";" << iterations << std::endl;
				
				printf("%*.*lf ms (float1, %6d samples)\n", 7, 2, (double)(clSystemTimeToMillis(endTime) - clSystemTimeToMillis(startTime)) / (double)iterations, iterations);
		
				unsigned int to_wait = rand() % 500;
				Sleep(to_wait);
				
				free(reference);
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

	for(unsigned int i = 0; i < memset_count; i++) {
		clFreeMemorySetting(settings[i]);
	}
	for(unsigned int i = 0; i < dcount; i++) {
		clFreeDeviceInfo(&devices[i]);
	}
	free(devices);

	std::cout << "DONE!" << std::endl;
	
	std::cout << "----- Saxpy test end - press any key to exit -----" << std::endl;
	getchar();
}

