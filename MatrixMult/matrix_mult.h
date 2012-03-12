#include <CLUtil.h>
#include <CLProgramOptions.h>
#include <CLMemorySetting.h>
#include <sstream>
#include "computation.h"

CLProgramOption min_matrix_size;
CLProgramOption max_matrix_size;
CLProgramOption block_size;
CLProgramOption tries;
CLProgramOption wait;
CLProgramOption local_size;
CLProgramOption cpu_threads;
CLProgramOption test_float_4;
CLProgramOption test_unoptimized;
CLProgramOption size_multiplier;
CLProgramOption log_file;
CLProgramOption append_to_file;
CLProgramOption test_duration;
CLProgramOption verify_output;
CLProgramOption simulate_read;

CLProgramOption test_cpu_sequential;
CLProgramOption test_cpu_threads;
CLProgramOption test_opencl_devices;
CLProgramOption test_heterogeneous;

CLProgramOption kernel_path;
CLProgramOption kernel_functions;
CLProgramOption kernel_options;
CLProgramOption kernel_profiling;

