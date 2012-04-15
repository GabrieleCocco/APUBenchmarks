#include "CLGeneral.h"
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>


/* End */
/* CL util functions */
CLUTILLIB_API int clRoundUp(int first, int second);
CLUTILLIB_API unsigned int clCpuCoreCount();
CLUTILLIB_API unsigned int clFact(unsigned int n);
CLUTILLIB_API unsigned int clCombination(unsigned int n, unsigned int k);
CLUTILLIB_API unsigned int** clCreateCombination(unsigned int dcount, unsigned int start_index, unsigned int dev_count);
/* End */