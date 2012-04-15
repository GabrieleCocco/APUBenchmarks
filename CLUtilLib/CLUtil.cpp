#include "CLUtil.h"

/* End */
CLUTILLIB_API int clRoundUp(int first, int second) {
	if(first % second == 0)
		return first;
	return ((first / second) + 1) * second;
}

CLUTILLIB_API unsigned int clCpuCoreCount() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	unsigned int numCPU = (unsigned int)sysinfo.dwNumberOfProcessors;
	return numCPU;
}

CLUTILLIB_API unsigned int clFact(unsigned int n) {
	unsigned int temp = 1;
	for(unsigned int i = n; i > 0; i--)
		temp *= i;
	return temp;
}

CLUTILLIB_API unsigned int clCombination(unsigned int n, unsigned int k) {
	if(n == 0 || k == 0 || n < k)
		return 0;
	return clFact(n) / (clFact(k) * clFact(n - k));
}


CLUTILLIB_API unsigned int** clCreateCombination(unsigned int dcount, unsigned int start_index, unsigned int dev_count) {
	unsigned int** device_list;
	if(dev_count == 1) {
		device_list = (unsigned int**)malloc((dcount - start_index) * sizeof(unsigned int*));
		for(unsigned int i = start_index; i < dcount; i++) {
			device_list[i - start_index] = (unsigned int*)malloc(sizeof(unsigned int));
			device_list[i - start_index][0] = i;
		}
		return device_list;
	}
	else {
		unsigned int combination_count = clCombination(dcount - start_index, dev_count);
		unsigned int** devices = (unsigned int**)malloc(combination_count * sizeof(unsigned int*));
		unsigned int combination_index = 0;

		for(unsigned int i = start_index; i < dcount; i++) {
			unsigned int sub_combination_count = clCombination(dcount - i - 1, dev_count - 1);
			if(sub_combination_count > 0) {
				unsigned int** subdevices = clCreateCombination(dcount, i + 1, dev_count - 1);
				for(unsigned int j = 0; j < sub_combination_count; j++) {
					devices[combination_index] = (unsigned int*)malloc(dev_count * sizeof(unsigned int));
					devices[combination_index][0] = i;
					for(unsigned int k = 0; k < dev_count - 1; k++)
						devices[combination_index][k + 1] = subdevices[j][k];
					free(subdevices[j]);
					combination_index++;
				}
				free(subdevices);
			}
		}
		return devices;
	}
}
