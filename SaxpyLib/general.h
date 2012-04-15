#ifdef SAXPYLIB_EXPORTS
#define SAXPYLIB_API __declspec(dllexport)
#else
#define SAXPYLIB_API __declspec(dllimport)
#endif

#include <CL/cl.hpp>