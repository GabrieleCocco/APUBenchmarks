#include "General.h"
#include <CLMemorySetting.h>
#include <CLUtil.h>
#include <sstream>
#include <string>
#include <direct.h>
#include "computations.h"

typedef SAXPYLIB_API enum SaxpyTestMode {
	TEST_MODE_HOST,
	TEST_MODE_DEVICE
} SaxpyTestMode;

typedef SAXPYLIB_API enum SaxpyMemoryMode {
	MEMORY_MODE_DEVICE_COPY,
	MEMORY_MODE_HOST_MAP,
	MEMORY_MODE_PERSISTENT_MEM_MAP
} SaxpyMemoryMode;

unsigned int vector_size;
unsigned int samples;
unsigned int use_float4;
unsigned int vector_multiplier;
unsigned int verify_output;

char* log_file;
unsigned int append_to_file;

SaxpyTestMode test_mode;
unsigned int host_threads;

char* kernel_path;
char* kernel_options;

unsigned int device_count;
unsigned int device_ids[MAX_DEV_COUNT];
unsigned int local_sizes[MAX_DEV_COUNT];
SaxpyMemoryMode input_memory_modes[MAX_DEV_COUNT];
SaxpyMemoryMode output_memory_modes[MAX_DEV_COUNT];
const char* kernel_functions[MAX_DEV_COUNT];

/* Global computation variables */
typedef int (*CLProfiledComputation) (void* data);
unsigned int platform_device_count;
CLDeviceInfo* platform_devices;

CLDeviceInfo* selected_devices;
SaxpyProfilingData pf_data;

SYSTEMTIME start, end;

CLProfiledComputation computation;
std::ofstream* output;
CLMemorySetting input_memory_settings[MAX_DEV_COUNT];
CLMemorySetting output_memory_settings[MAX_DEV_COUNT];
int result;

/* Callback */
typedef void (*SaxpyCallback) (SYSTEMTIME start, SYSTEMTIME end, int result);

/* API functions */
SAXPYLIB_API void run(SaxpyCallback callback);
SAXPYLIB_API void setup(int argc, char* argv[], cl_command_queue** queues, unsigned int* queues_count);

/* NON-API functions */
void finish(SaxpyCallback callback);