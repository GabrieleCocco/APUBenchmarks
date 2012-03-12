#include <CLMemorySetting.h>
#include <CLProgramOptions.h>
#include <sstream>
#include <string>
#include <direct.h>
#include "computations.h"

CLProgramOption min_vector_size;
CLProgramOption max_vector_size;
CLProgramOption tries;
CLProgramOption increment;
CLProgramOption wait;
CLProgramOption cpu_threads;
CLProgramOption test_int_4;
CLProgramOption test_duration;
CLProgramOption verify_output;
CLProgramOption reduce_limit;

CLProgramOption test_host;
CLProgramOption test_opencl_devices;
CLProgramOption min_devices;
CLProgramOption max_devices;

CLProgramOption log_file;
CLProgramOption append_to_file;
CLProgramOption local_size;
CLProgramOption kernel_path;
CLProgramOption kernel_functions;
CLProgramOption kernel_options;
CLProgramOption kernel_profiling;

