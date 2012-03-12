#ifndef CL_UTIL_H
#define CL_UTIL_H

#include <Windows.h>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

typedef struct Timer {
	LARGE_INTEGER frequency;
	LARGE_INTEGER start_time;
	void start() {
		QueryPerformanceFrequency(&frequency);	
		QueryPerformanceCounter(&start_time);
	}
	double get() {
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		double elapsedTime = (end.QuadPart - start_time.QuadPart) * 1000.0 / frequency.QuadPart;
		return elapsedTime;
	}
} Timer;

/* End */
/* CL util functions */
int clRoundUp(int first, int second);
long clSystemTimeToMillis(SYSTEMTIME time);
unsigned int clCpuCoreCount();
unsigned int clFact(unsigned int n);
unsigned int clCombination(unsigned int n, unsigned int k);
unsigned int** clCreateCombination(unsigned int dcount, unsigned int start_index, unsigned int dev_count);
/* End */

#endif