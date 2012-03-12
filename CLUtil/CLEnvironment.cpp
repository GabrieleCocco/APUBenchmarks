#include "CLEnvironment.h"

void clLoadFile(
	const char* file,
	unsigned int* size,
	const char* * output) {
    // locals 
    FILE* pFileStream = NULL;
    size_t szSourceLength;

	if(fopen_s(&pFileStream, file, "rb") != 0) 
		*output = NULL;

    // get the length of the source code
    fseek(pFileStream, 0, SEEK_END); 
    szSourceLength = ftell(pFileStream);
    fseek(pFileStream, 0, SEEK_SET); 

    // allocate a buffer for the source code string and read it in
    char* cSourceString = (char*)malloc(szSourceLength + 1); 
    if (fread(cSourceString, szSourceLength, 1, pFileStream) != 1)
    {
        fclose(pFileStream);
        free(cSourceString);
    }

    // close the file and return the total length of the combined (preamble + source) string
    fclose(pFileStream);
    cSourceString[szSourceLength] = '\0';
	*size = szSourceLength;
    *output = cSourceString;
}

void clListDevices(
	unsigned int* count,
	CLDeviceInfo** output) {

	cl_device_id device_ids[100];
	CLDeviceInfo* devices;
	cl_platform_id platform;
	clGetPlatformIDs(1, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 100, device_ids, count);
	
	devices = (CLDeviceInfo*)malloc(*count * sizeof(CLDeviceInfo));
	for(unsigned int i = 0; i < *count; i++) {
		devices[i].id = device_ids[i];
		devices[i].name = (char*)malloc(256 * sizeof(char));
		cl_device_type type;
		clGetDeviceInfo(devices[i].id, CL_DEVICE_NAME, 256, devices[i].name, NULL);		
		clGetDeviceInfo(devices[i].id, CL_DEVICE_TYPE, sizeof(type), &type, NULL);
		devices[i].type = type;
 	}
	*output = devices;
}

void clPrintDeviceList(
	CLDeviceInfo* devices,
	unsigned int device_count,
	const char* before_item) {
	for(unsigned int i = 0; i < device_count; i++) {
		char* device_type = NULL;
		clDeviceTypeToString(devices[i].type, &device_type);
		std::cout << before_item << devices[i].name << " [" << device_type << "]" << std::endl;
		free(device_type);
	}
}

void clGetDevice(
	const char* name,
	CLDeviceInfo* device,
	unsigned int* found) {

	unsigned int device_count = 0;
	CLDeviceInfo* devices;
	clListDevices(&device_count, &devices);
	*found = false;
	for(unsigned int i = 0; i < device_count; i++) {
		if(strncmp(devices[i].name, name, strlen(name)) == 0) {
			device->id = devices[i].id;
			strncpy(device->name, devices[i].name, strlen(name));
			device->type = devices[i].type;
			*found = true;
		}
	}
	for(unsigned int i = 0; i < device_count; i++) 
		free(devices[i].name);
	free(devices);
}

void clFreeDeviceInfo(
	CLDeviceInfo* device) {	
	free(device->name);
}

void clDeviceTypeToString(
	cl_device_type type, 
	char** output) {

	char* temp = "DEFAULT";
	switch(type) {
		case CL_DEVICE_TYPE_GPU: {
			temp = "GPU";
		} break;
		case CL_DEVICE_TYPE_CPU: {
			temp = "CPU";
		} break;
		case CL_DEVICE_TYPE_ACCELERATOR: {
			temp = "ACCELERATOR";
		} break;
		default: {
			temp = "DEFAULT";
		}
	}
	*output = (char*)malloc(strlen(temp) + 1);
	memset(*output, 0, strlen(temp) + 1);
	strncpy(*output, temp, strlen(temp));
}

void clCreateDeviceEnvironment(
	CLDeviceInfo* devices,
	unsigned int device_count,
	const char* kernel_path,
	const char** kernel_functions,
	unsigned int kernel_functions_count,
	const char* build_options,
	unsigned int enable_gpu_profiling,
	unsigned int shared,
	CLDeviceEnvironment* output) {
		
	cl_platform_id platform_id;
	printf("\n  %-35s", "Getting platform id...");
	clGetPlatformIDs(1, &platform_id, NULL);
	printf("DONE!\n");

	if(!shared) {
		for(unsigned int i = 0; i < device_count; i++) {
			output[i].platform = platform_id;

			printf("  %-35s", "Creating context...");
			output[i].info.id = devices[i].id;
			output[i].info.type = devices[i].type;
			output[i].info.name = (char*)malloc(strlen(devices[i].name) + 1);
			memset(output[i].info.name, 0, strlen(devices[i].name) + 1);
			strncpy(output[i].info.name, devices[i].name, strlen(devices[i].name));
			
			output[i].context = clCreateContext(0, 1, &devices[i].id, NULL, NULL, NULL);
			printf("DONE!\n");

			printf("  %-35s", "Creating command queues...");	
			if(enable_gpu_profiling)
				output[i].queue = clCreateCommandQueue(output[i].context, output[i].info.id, CL_QUEUE_PROFILING_ENABLE, NULL);
			else
				output[i].queue = clCreateCommandQueue(output[i].context, output[i].info.id, 0, NULL);
			printf("DONE!\n");

			printf("  %-35s", "Loading kernel file...");
			const char* kernel_src;
			unsigned int kernel_size;
			clLoadFile(kernel_path, &kernel_size, &kernel_src);
			printf("DONE!\n");

			printf("  %-35s", "Creating program with source...");
			output[i].program = clCreateProgramWithSource(output[i].context, 1, &kernel_src, &kernel_size, NULL);
			printf("DONE!\n");

			printf("  %-35s", "Building program...");
			cl_int status = clBuildProgram(output[i].program, 0, NULL, build_options, NULL, NULL);
			if(status != CL_SUCCESS) {
				char build_log[2048];
				memset(build_log, 0, 2048);
				clGetProgramBuildInfo(output[i].program, devices[i].id, CL_PROGRAM_BUILD_LOG, 2048, build_log, NULL);
				printf("\n\nBUILD LOG: \n %s\n\n", build_log);
			}
			printf("DONE!\n");

			output[i].kernels_count = kernel_functions_count;
			output[i].kernels = (cl_kernel*)malloc(kernel_functions_count * sizeof(cl_kernel));
			for(unsigned int j = 0; j < kernel_functions_count; j++) {
				cl_int err;
		
				std::ostringstream o;
				o << "  Creating kernel " << kernel_functions[j];

				printf("  %-35s", o.str().c_str());
				output[i].kernels[j] = clCreateKernel(output[i].program, kernel_functions[j], &err);
				if(err == 0)
					printf("DONE!\n");
				else
					printf("FAIL! (%d)\n", err);
			}

			free((char*)kernel_src);
		}
	}
	else {
		cl_context context;
		cl_program program;
		cl_kernel* kernels;
	
		printf("  %-35s", "Creating context...");
		cl_device_id* ids = (cl_device_id*)malloc(device_count * sizeof(cl_device_id));
		for(unsigned int i = 0; i < device_count; i++) 
			ids[i] = devices[i].id;
		context = clCreateContext(0, device_count, ids, NULL, NULL, NULL);
		free(ids);
		printf("DONE!\n");
		
		printf("  %-35s", "Creating command queues...");	
		for(unsigned int i = 0; i < device_count; i++) {
			if(enable_gpu_profiling)
				output[i].queue = clCreateCommandQueue(context, output[i].info.id, CL_QUEUE_PROFILING_ENABLE, NULL);
			else
				output[i].queue = clCreateCommandQueue(context, output[i].info.id, 0, NULL);
		}
		printf("DONE!\n");

		printf("  %-35s", "Loading kernel file...");
		const char* kernel_src;
		unsigned int kernel_size;
		clLoadFile(kernel_path, &kernel_size, &kernel_src);
		printf("DONE!\n");

		printf("  %-35s", "Creating program with source...");
		program = clCreateProgramWithSource(context, 1, &kernel_src, &kernel_size, NULL);		
		printf("DONE!\n");

		printf("  %-35s", "Building program...");
		cl_int status = clBuildProgram(program, 0, NULL, build_options, NULL, NULL);
		if(status != CL_SUCCESS) {
			char build_log[2048];
			memset(build_log, 0, 2048);
			clGetProgramBuildInfo(program, devices[0].id, CL_PROGRAM_BUILD_LOG, 2048, build_log, NULL);
			printf("\n\nBUILD LOG: \n %s\n\n", build_log);
		}		
		printf("DONE!\n");

		kernels = (cl_kernel*)malloc(kernel_functions_count * sizeof(cl_kernel));
		for(unsigned int j = 0; j < kernel_functions_count; j++) {
			cl_int err;
		
			std::ostringstream o;
			o << "  Creating kernel " << kernel_functions[j];

			printf("  %-35s", o.str().c_str());
			kernels[j] = clCreateKernel(program, kernel_functions[j], &err);
			if(err == 0)
				printf("DONE!\n");
			else
				printf("FAIL! (%d)\n", err);
		}
		free((char*)kernel_src);

		for(unsigned int i = 0; i < device_count; i++) {
			output[i].platform = platform_id;
			output[i].context = context;
			output[i].program = program;
			output[i].kernels_count = kernel_functions_count;
			output[i].kernels = (cl_kernel*)malloc(kernel_functions_count * sizeof(cl_kernel));
			for(unsigned int j = 0; j < kernel_functions_count; j++) 
				output[i].kernels[j] = kernels[j];
			output[i].info.id = devices[i].id;
			output[i].info.type = devices[i].type;
			output[i].info.name = (char*)malloc(strlen(devices[i].name) + 1);
			memset(output[i].info.name, 0, strlen(devices[i].name) + 1);
			strncpy(output[i].info.name, devices[i].name, strlen(devices[i].name));
		}		
		free(kernels);
	}
}

void clFreeDeviceEnvironments(
	CLDeviceEnvironment* environments,
	unsigned int device_count,
	unsigned int shared) {
	if(!shared) {
		for(unsigned int i = 0 ; i < device_count; i++) {
			if(environments[i].context)
				clReleaseContext(environments[i].context);
			if(environments[i].program)
				clReleaseProgram(environments[i].program);
			if(environments[i].kernels) {
				for(unsigned int j = 0; j < environments[i].kernels_count; j++) {
					if(environments[i].kernels[j])
						clReleaseKernel(environments[i].kernels[j]);  
				}
				free(environments[i].kernels);
			}
			if(environments[i].queue) 
				clReleaseCommandQueue(environments[i].queue);
			clFreeDeviceInfo(&environments[i].info);
		}
	}
	else {
		clReleaseContext(environments[0].context);
		clReleaseProgram(environments[0].program);
		for(unsigned int j = 0; j < environments[0].kernels_count; j++) {
			if(environments[0].kernels[j])
				clReleaseKernel(environments[0].kernels[j]);  
		}
		clReleaseCommandQueue(environments[0].queue);
		
		for(unsigned int i = 0 ; i < device_count; i++) {
			free(environments[i].kernels);		
			clFreeDeviceInfo(&environments[i].info);
		}
	}
}

void clMemFlagsToString(
	cl_mem_flags flags,
	char* output) {
	
	output = (char*)malloc(256);
	memset(output, 0, 256);
	unsigned int pointer = 0;
#ifdef CL_MEM_USE_PERSISTENT_MEM_AMD
	if(flags & CL_MEM_USE_PERSISTENT_MEM_AMD) {
		char* temp = "CL_MEM_USE_PERSISTENT_MEM_AMD | ";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
#endif
	if(flags & CL_MEM_ALLOC_HOST_PTR) {
		char* temp = "CL_MEM_ALLOC_HOST_PTR | ";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
	if(flags & CL_MEM_USE_HOST_PTR) {
		char* temp = "CL_MEM_USE_HOST_PTR | ";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
	if(flags & CL_MEM_COPY_HOST_PTR) {
		char* temp = "CL_MEM_COPY_HOST_PTR | ";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
	if(flags & CL_MEM_READ_ONLY) {
		char* temp = "CL_MEM_READ_ONLY";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
	if(flags & CL_MEM_WRITE_ONLY) {
		char* temp = "CL_MEM_WRITE_ONLY";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
	if(flags & CL_MEM_READ_WRITE) {
		char* temp = "CL_MEM_READ_WRITE";
		strncpy(output + pointer, temp, strlen(temp));	
		pointer += strlen(temp);
	}
}

void clErrorToString(
	cl_int error,
	char* output) {
    static const char* errorString[] = {
        "CL_SUCCESS",
        "CL_DEVICE_NOT_FOUND",
        "CL_DEVICE_NOT_AVAILABLE",
        "CL_COMPILER_NOT_AVAILABLE",
        "CL_MEM_OBJECT_ALLOCATION_FAILURE",
        "CL_OUT_OF_RESOURCES",
        "CL_OUT_OF_HOST_MEMORY",
        "CL_PROFILING_INFO_NOT_AVAILABLE",
        "CL_MEM_COPY_OVERLAP",
        "CL_IMAGE_FORMAT_MISMATCH",
        "CL_IMAGE_FORMAT_NOT_SUPPORTED",
        "CL_BUILD_PROGRAM_FAILURE",
        "CL_MAP_FAILURE",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "CL_INVALID_VALUE",
        "CL_INVALID_DEVICE_TYPE",
        "CL_INVALID_PLATFORM",
        "CL_INVALID_DEVICE",
        "CL_INVALID_CONTEXT",
        "CL_INVALID_QUEUE_PROPERTIES",
        "CL_INVALID_COMMAND_QUEUE",
        "CL_INVALID_HOST_PTR",
        "CL_INVALID_MEM_OBJECT",
        "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
        "CL_INVALID_IMAGE_SIZE",
        "CL_INVALID_SAMPLER",
        "CL_INVALID_BINARY",
        "CL_INVALID_BUILD_OPTIONS",
        "CL_INVALID_PROGRAM",
        "CL_INVALID_PROGRAM_EXECUTABLE",
        "CL_INVALID_KERNEL_NAME",
        "CL_INVALID_KERNEL_DEFINITION",
        "CL_INVALID_KERNEL",
        "CL_INVALID_ARG_INDEX",
        "CL_INVALID_ARG_VALUE",
        "CL_INVALID_ARG_SIZE",
        "CL_INVALID_KERNEL_ARGS",
        "CL_INVALID_WORK_DIMENSION",
        "CL_INVALID_WORK_GROUP_SIZE",
        "CL_INVALID_WORK_ITEM_SIZE",
        "CL_INVALID_GLOBAL_OFFSET",
        "CL_INVALID_EVENT_WAIT_LIST",
        "CL_INVALID_EVENT",
        "CL_INVALID_OPERATION",
        "CL_INVALID_GL_OBJECT",
        "CL_INVALID_BUFFER_SIZE",
        "CL_INVALID_MIP_LEVEL",
        "CL_INVALID_GLOBAL_WORK_SIZE",
    };

    const int errorCount = sizeof(errorString)/sizeof(errorString[0]);
	const int index = -error;
	if (index >= 0 && index < errorCount) {
		output = (char*)malloc(strlen(errorString[index]) + 1);
		strncpy(output, errorString[index], strlen(errorString[index]));
		output[strlen(errorString[index])] = 0;
	}
	else {
		output = (char*)malloc(strlen("Unspecified Error") + 1);
		strncpy(output, "Unspecified Error", strlen("Unspecified Error"));
		output[strlen("Unspecified Error")] = 0;
	}
}

void clPrintDeviceInfo(
	cl_device_id device)
{
    char device_string[1024];
    bool nv_device_attibute_query = false;

    // CL_DEVICE_NAME
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_string), &device_string, NULL);
    std::cout << "CL_DEVICE_NAME: " << device_string << std::endl;

    // CL_DEVICE_VENDOR
    clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_string), &device_string, NULL);
    std::cout << "CL_DEVICE_VENDOR: " << device_string << std::endl;

    // CL_DRIVER_VERSION
    clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(device_string), &device_string, NULL);
    std::cout << "CL_DRIVER_VERSION: " << device_string << std::endl;

    // CL_DEVICE_VERSION
    clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(device_string), &device_string, NULL);
    std::cout << "CL_DEVICE_VERSION: " << device_string << std::endl;

#if !defined(__APPLE__) && !defined(__MACOSX)
    // CL_DEVICE_OPENCL_C_VERSION (if CL_DEVICE_VERSION version > 1.0)
    if(strncmp("OpenCL 1.0", device_string, 10) != 0) 
    {
        #ifndef CL_DEVICE_OPENCL_C_VERSION
            #define CL_DEVICE_OPENCL_C_VERSION 0x103D   
        #endif

        clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, sizeof(device_string), &device_string, NULL);
        std::cout << "CL_DEVICE_OPENCL_C_VERSION: " << device_string << std::endl;
    }
#endif

    // CL_DEVICE_TYPE
    cl_device_type type;
    clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(type), &type, NULL);
    if( type & CL_DEVICE_TYPE_CPU )
        std::cout << "CL_DEVICE_TYPE: " << "CL_DEVICE_TYPE_CPU" << std::endl;
    if( type & CL_DEVICE_TYPE_GPU )
        std::cout << "CL_DEVICE_TYPE: " << "CL_DEVICE_TYPE_GPU" << std::endl;
    if( type & CL_DEVICE_TYPE_ACCELERATOR )
        std::cout << "CL_DEVICE_TYPE: " << "CL_DEVICE_TYPE_ACCELERATOR" << std::endl;
    if( type & CL_DEVICE_TYPE_DEFAULT )
        std::cout << "CL_DEVICE_TYPE: " << "CL_DEVICE_TYPE_DEFAULT" << std::endl;
    
    // CL_DEVICE_MAX_COMPUTE_UNITS
    cl_uint compute_units;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    std::cout << "CL_DEVICE_MAX_COMPUTE_UNITS: " << compute_units << std::endl;

	// CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS
    size_t workitem_dims;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(workitem_dims), &workitem_dims, NULL);
    std::cout << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: " << workitem_dims << std::endl;

    // CL_DEVICE_MAX_WORK_ITEM_SIZES
    size_t workitem_size[3];
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(workitem_size), &workitem_size, NULL);
    std::cout << "CL_DEVICE_MAX_WORK_ITEM_SIZES: " << workitem_size[0] << ", " << workitem_size[1] << ", " << workitem_size[2] << std::endl;
    
    // CL_DEVICE_MAX_WORK_GROUP_SIZE
    size_t workgroup_size;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(workgroup_size), &workgroup_size, NULL);
    std::cout << "CL_DEVICE_MAX_WORK_GROUP_SIZE: " << workgroup_size << std::endl;

    // CL_DEVICE_MAX_CLOCK_FREQUENCY
    cl_uint clock_frequency;
    clGetDeviceInfo(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clock_frequency), &clock_frequency, NULL);
    std::cout << "CL_DEVICE_MAX_CLOCK_FREQUENCY: " << clock_frequency << "Mhz" << std::endl;

    // CL_DEVICE_ADDRESS_BITS
    cl_uint addr_bits;
    clGetDeviceInfo(device, CL_DEVICE_ADDRESS_BITS, sizeof(addr_bits), &addr_bits, NULL);
    std::cout << "CL_DEVICE_ADDRESS_BITS: " << addr_bits << std::endl;

    // CL_DEVICE_MAX_MEM_ALLOC_SIZE
    cl_ulong max_mem_alloc_size;
    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_mem_alloc_size), &max_mem_alloc_size, NULL);
    std::cout << "CL_DEVICE_MAX_MEM_ALLOC_SIZE: " << (unsigned int)(max_mem_alloc_size / (1024 * 1024)) << "MByte" << std::endl;

    // CL_DEVICE_GLOBAL_MEM_SIZE
    cl_ulong mem_size;
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem_size), &mem_size, NULL);
    std::cout << "CL_DEVICE_GLOBAL_MEM_SIZE: " << (unsigned int)(mem_size / (1024 * 1024)) << "MByte" << std::endl;

    // CL_DEVICE_ERROR_CORRECTION_SUPPORT
    cl_bool error_correction_support;
    clGetDeviceInfo(device, CL_DEVICE_ERROR_CORRECTION_SUPPORT, sizeof(error_correction_support), &error_correction_support, NULL);
	if(error_correction_support == CL_TRUE)
		std::cout << "CL_DEVICE_ERROR_CORRECTION_SUPPORT: yes" << std::endl;
	else
		std::cout << "CL_DEVICE_ERROR_CORRECTION_SUPPORT: no" << std::endl;

    // CL_DEVICE_LOCAL_MEM_TYPE
    cl_device_local_mem_type local_mem_type;
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_TYPE, sizeof(local_mem_type), &local_mem_type, NULL);
	if(local_mem_type == 1)
		std::cout << "CL_DEVICE_LOCAL_MEM_TYPE: local" << std::endl;
	else
		std::cout << "CL_DEVICE_LOCAL_MEM_TYPE: global" << std::endl;

    // CL_DEVICE_LOCAL_MEM_SIZE
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(mem_size), &mem_size, NULL);
    std::cout << "CL_DEVICE_LOCAL_MEM_SIZE: " << (unsigned int)(mem_size / 1024) << "KByte" << std::endl;

    // CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE
    clGetDeviceInfo(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, sizeof(mem_size), &mem_size, NULL);
    std::cout << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: " << (unsigned int)(mem_size / 1024) << "KByte" << std::endl;

    // CL_DEVICE_QUEUE_PROPERTIES
    cl_command_queue_properties queue_properties;
    clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(queue_properties), &queue_properties, NULL);
    if( queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
        std::cout << "CL_DEVICE_QUEUE_PROPERTIES: " << "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE" << std::endl;    
    if( queue_properties & CL_QUEUE_PROFILING_ENABLE )
        std::cout << "CL_DEVICE_QUEUE_PROPERTIES:" << "CL_QUEUE_PROFILING_ENABLE" << std::endl;

    // CL_DEVICE_IMAGE_SUPPORT
    cl_bool image_support;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT, sizeof(image_support), &image_support, NULL);
    std::cout << "CL_DEVICE_IMAGE_SUPPORT: " << image_support << std::endl;

    // CL_DEVICE_MAX_READ_IMAGE_ARGS
    cl_uint max_read_image_args;
    clGetDeviceInfo(device, CL_DEVICE_MAX_READ_IMAGE_ARGS, sizeof(max_read_image_args), &max_read_image_args, NULL);
    std::cout << "CL_DEVICE_MAX_READ_IMAGE_ARGS: " << max_read_image_args << std::endl;

    // CL_DEVICE_MAX_WRITE_IMAGE_ARGS
    cl_uint max_write_image_args;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WRITE_IMAGE_ARGS, sizeof(max_write_image_args), &max_write_image_args, NULL);
    std::cout << "CL_DEVICE_MAX_WRITE_IMAGE_ARGS: " << max_write_image_args << std::endl;

    // CL_DEVICE_SINGLE_FP_CONFIG
    cl_device_fp_config fp_config;
    clGetDeviceInfo(device, CL_DEVICE_SINGLE_FP_CONFIG, sizeof(cl_device_fp_config), &fp_config, NULL);
    std::cout << "CL_DEVICE_SINGLE_FP_CONFIG: " << (fp_config & CL_FP_DENORM ? "denorms " : "") << (fp_config & CL_FP_INF_NAN ? "INF-quietNaNs " : "") << (fp_config & CL_FP_ROUND_TO_NEAREST ? "round-to-nearest " : "") << (fp_config & CL_FP_ROUND_TO_ZERO ? "round-to-zero " : "") << (fp_config & CL_FP_ROUND_TO_INF ? "round-to-inf " : "") << (fp_config & CL_FP_FMA ? "fma " : "") << std::endl;
    
    // CL_DEVICE_IMAGE2D_MAX_WIDTH, CL_DEVICE_IMAGE2D_MAX_HEIGHT, CL_DEVICE_IMAGE3D_MAX_WIDTH, CL_DEVICE_IMAGE3D_MAX_HEIGHT, CL_DEVICE_IMAGE3D_MAX_DEPTH
    size_t szMaxDims[5];
    std::cout << "CL_DEVICE_IMAGE" << std::endl; 
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(size_t), &szMaxDims[0], NULL);
    std::cout << "\t2D_MAX_WIDTH" << szMaxDims[0] << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t), &szMaxDims[1], NULL);
    std::cout << "\t2D_MAX_HEIGHT\t %u\n" << szMaxDims[1] << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_WIDTH, sizeof(size_t), &szMaxDims[2], NULL);
    std::cout << "\t3D_MAX_WIDTH\t %u\n" << szMaxDims[2] << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_HEIGHT, sizeof(size_t), &szMaxDims[3], NULL);
    std::cout << "\t3D_MAX_HEIGHT\t %u\n" << szMaxDims[3] << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_IMAGE3D_MAX_DEPTH, sizeof(size_t), &szMaxDims[4], NULL);
    std::cout << "\t3D_MAX_DEPTH\t %u\n" << szMaxDims[4] << std::endl;

    // CL_DEVICE_EXTENSIONS: get device extensions, and if any then parse & log the string onto separate lines
    clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof(device_string), &device_string, NULL);
    if (device_string != 0) 
    {
        std::cout << "CL_DEVICE_EXTENSIONS:" << std::endl;
        std::string stdDevString;
        stdDevString = std::string(device_string);
        size_t szOldPos = 0;
        size_t szSpacePos = stdDevString.find(' ', szOldPos); // extensions string is space delimited
        while (szSpacePos != stdDevString.npos)
        {
            if( strcmp("cl_nv_device_attribute_query", stdDevString.substr(szOldPos, szSpacePos - szOldPos).c_str()) == 0 )
                nv_device_attibute_query = true;

            if (szOldPos > 0)
            {
                std::cout << "\t\t" << std::endl;
            }
            std::cout << "" << stdDevString.substr(szOldPos, szSpacePos - szOldPos).c_str();
            
            do {
                szOldPos = szSpacePos + 1;
                szSpacePos = stdDevString.find(' ', szOldPos);
            } while (szSpacePos == szOldPos);
        }
        std::cout <<  std::endl;
    }
    else 
    {
        std::cout << "CL_DEVICE_EXTENSIONS: None" << std::endl;
    }

    // CL_DEVICE_PREFERRED_VECTOR_WIDTH_<type>
    std::cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_<t>" << std::endl; 
    cl_uint vec_width [6];
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, sizeof(cl_uint), &vec_width[0], NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, sizeof(cl_uint), &vec_width[1], NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, sizeof(cl_uint), &vec_width[2], NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, sizeof(cl_uint), &vec_width[3], NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, sizeof(cl_uint), &vec_width[4], NULL);
    clGetDeviceInfo(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, sizeof(cl_uint), &vec_width[5], NULL);
    std::cout << "CHAR " << vec_width[0] << ", SHORT " << vec_width[1] << ", INT " << vec_width[2] << ", LONG " << vec_width[3] << ", FLOAT " << vec_width[4] << ", DOUBLE " << vec_width[5] << std::endl;
}
