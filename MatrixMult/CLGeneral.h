#ifdef CLUTILLIB_EXPORTS
#define CLUTILLIB_API __declspec(dllexport)
#else
#define CLUTILLIB_API __declspec(dllimport)
#endif

#include <CL/cl.hpp>