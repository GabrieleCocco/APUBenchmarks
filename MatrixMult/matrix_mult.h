#include <CLUtil.h>
#include <CLProgramOptions.h>
#include <CLMemorySetting.h>
#include <sstream>
#include "computation.h"

typedef struct PerformanceScaleInfo {
	cl_device_id device;
	double completion_time;
} PerformanceScaleInfo;

CLProgramOption min_matrix_size;
CLProgramOption max_matrix_size;
CLProgramOption tries;
CLProgramOption wait;
CLProgramOption local_size;
CLProgramOption cpu_threads;
CLProgramOption size_multiplier;
CLProgramOption log_file;
CLProgramOption append_to_file;
CLProgramOption test_duration;
CLProgramOption verify_output;
CLProgramOption simulate_read;

CLProgramOption test_host;
CLProgramOption test_devices;
CLProgramOption min_devices;
CLProgramOption max_devices;

CLProgramOption kernel_path;
CLProgramOption kernel_functions;
CLProgramOption kernel_options;
CLProgramOption kernel_profiling;

