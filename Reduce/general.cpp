#include "general.h"

void computeInput(int* data, unsigned int size, unsigned int offset) {
	for(unsigned int i = 0; i < size; i++)
		data[i] = (int)(1);
}

int computeOutput(int* data, unsigned int size) {
	int value = 0;
	for(unsigned int i = 0; i < size; i++)
		value += data[i];
	return value;
}

int compare(int first, int second) {
	if(first == second)
		return 0;
	return -1;
}