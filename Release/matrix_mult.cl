/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */
#define DATATYPE float
/* Matrix multiplication: C = A * B.
 * Device code.
 */
#define AS(i, j) As[j + i * BLOCK_SIZE]
#define BS(i, j) Bs[j + i * BLOCK_SIZE]

///////////////////////////////////////////////////////////////////////////////
//! Matrix multiplication on the device: C = A * B
//! uiWA is A's width and uiWB is B's width
////////////////////////////////////////////////////////////////////////////////
kernel void matrixMul(__global float* A, __global float* B, __global float* C, int uiWA, int hA, int uiWB) {
	// Block index
    int bx = get_group_id(0);
    int by = get_group_id(1);

    // Thread index
    int tx = get_local_id(0);
    int ty = get_local_id(1);

	//Global index
	int gx = get_global_id(0);
	int gy = get_global_id(1);

	// Index of the first sub-matrix of A processed by the block
	int aBegin = uiWA * BLOCK_SIZE * by;
	int heightBegin = by * BLOCK_SIZE;

	// Index of the last sub-matrix of A processed by the block
	int aEnd   = aBegin + uiWA - 1;

	// Step size used to iterate through the sub-matrices of A
	int aStep  = BLOCK_SIZE;

	// Index of the first sub-matrix of B processed by the block
	int bBegin = BLOCK_SIZE * bx;

	// Step size used to iterate through the sub-matrices of B
	int bStep  = BLOCK_SIZE * uiWB;

	// Csub is used to store the element of the block sub-matrix
	// that is computed by the thread
	float Csub = 0.0f;
	
	// Declaration of the local memory array As 
	// used to store the sub-matrix of A
	__local DATATYPE As[BLOCK_SIZE][BLOCK_SIZE];
 
	// Declaration of the local memory array Bs 
	// used to store the sub-matrix of B
	__local DATATYPE Bs[BLOCK_SIZE][BLOCK_SIZE];

	// Loop over all the sub-matrices of A and B
	// required to compute the block sub-matrix
	for (int a = aBegin, b = bBegin;
				a <= aEnd;
				a += aStep, b += bStep) {

		// Load the matrices from device memory
		// to shared memory; each thread loads
		// one element of each matrix
		As[ty][tx] = A[a + uiWA * ty + tx];
		Bs[ty][tx] = B[b + uiWB * ty + tx];

		// Synchronize to make sure the matrices are loaded
		barrier(CLK_LOCAL_MEM_FENCE);

		// Multiply the two matrices together;
		// each thread computes one element
		// of the block sub-matrix  
		#pragma unroll BLOCK_SIZE
		for (int k = 0; k < BLOCK_SIZE; ++k)
			Csub += As[ty][k] * Bs[k][tx];

		// Synchronize to make sure that the preceding
		// computation is done before loading two new
		// sub-matrices of A and B in the next iteration
		barrier(CLK_LOCAL_MEM_FENCE);
	}

	// Write the block sub-matrix to device memory;
	// each thread writes one element
	C[gy * get_global_size(0) + gx] = Csub;

}

// kernel.cl
// Multiply two matrices A * B = C
// Device code.
  
// OpenCL Kernel
__kernel void matrixMulBase(__global DATATYPE* A, __global DATATYPE* B, __global DATATYPE* C, int wA, int hA, int wB) { 
	// 2D Thread ID
	int tx = get_global_id(0);
	int ty = get_global_id(1);
 
	if(ty < hA && tx < wB) {
		// value stores the element 
		// that is computed by the thread
		DATATYPE value = 0;
		#pragma unroll
		for (int k = 0; k < wA; ++k)
		{
			DATATYPE elementA = A[ty * wA + k];
			DATATYPE elementB = B[k * wB + tx];
			value += elementA * elementB;
		}
		// Write the matrix to device memory each 
		// thread writes one element
		C[ty * wB + tx] = value;
	}
}