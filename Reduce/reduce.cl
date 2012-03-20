__kernel void reduce0(__global int *g_idata, __local int* sdata, unsigned int n, __global int *g_odata)
{
    // load shared mem
    unsigned int tid = get_local_id(0);
    unsigned int i = get_global_id(0);
    
    sdata[tid] = (i < n) ? g_idata[i] : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);

    // do reduction in shared mem
    for(unsigned int s=1; s < get_local_size(0); s *= 2) {
        // modulo arithmetic is slow!
        if ((tid % (2*s)) == 0)
            sdata[tid] += sdata[tid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}


/* This version uses contiguous threads, but its interleaved 
   addressing results in many shared memory bank conflicts. */
__kernel void reduce1(__global int *g_idata, __local int* sdata, unsigned int n, __global int *g_odata)
{
    // load shared mem
    unsigned int tid = get_local_id(0);
    unsigned int i = get_global_id(0);
    
    sdata[tid] = (i < n) ? g_idata[i] : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);

    // do reduction in shared mem
    for(unsigned int s=1; s < get_local_size(0); s *= 2) 
    {
        int index = 2 * s * tid;

        if (index < get_local_size(0)) 
            sdata[index] += sdata[index + s];

        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}

/*
    This version uses sequential addressing -- no divergence or bank conflicts.
*/
__kernel void reduce2(__global int *g_idata, __local int* sdata, unsigned int n, __global int *g_odata)
{
    // load shared mem
    unsigned int tid = get_local_id(0);
    unsigned int i = get_global_id(0);
    
    sdata[tid] = (i < n) ? g_idata[i] : 0;
    
    barrier(CLK_LOCAL_MEM_FENCE);

    // do reduction in shared mem
    for(unsigned int s=get_local_size(0)/2; s>0; s>>=1) 
    {
        if (tid < s) 
            sdata[tid] += sdata[tid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}

/*
    This version uses n/2 threads --
    it performs the first level of reduction when reading from global memory
*/
__kernel void reduce3(__global int *g_idata, __local int* sdata, unsigned int n, __global int *g_odata)
{
    // perform first level of reduction,
    // reading from global memory, writing to shared memory
    unsigned int tid = get_local_id(0);
    unsigned int i = get_group_id(0)*(get_local_size(0)*2) + get_local_id(0);

    sdata[tid] = (i < n) ? g_idata[i] : 0;
    if (i + get_local_size(0) < n) 
        sdata[tid] += g_idata[i+get_local_size(0)];  

    barrier(CLK_LOCAL_MEM_FENCE);

    // do reduction in shared mem
    for(unsigned int s=(get_local_size(0) >> 1); s>0; s>>=1) 
    {
        if (tid < s) 
            sdata[tid] += sdata[tid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem 
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}

__kernel void reduce3_int4(__global int4 *g_idata, __local int4* sdata, unsigned int n, __global int4 *g_odata)
{
    // perform first level of reduction,
    // reading from global memory, writing to shared memory
    unsigned int tid = get_local_id(0);
    unsigned int i = get_group_id(0)*(get_local_size(0)*2) + get_local_id(0);

    sdata[tid] = (i < n) ? g_idata[i] : 0;
    if (i + get_local_size(0) < n) 
        sdata[tid] += g_idata[i+get_local_size(0)];  

    barrier(CLK_LOCAL_MEM_FENCE);

    // do reduction in shared mem
    for(unsigned int s=(get_local_size(0) >> 1); s>0; s>>=1) 
    {
        if (tid < s) 
            sdata[tid] += sdata[tid + s];
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    // write result for this block to global mem 
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}

__kernel void reduce4(__global int *g_idata, __local int* sdata, unsigned int n, __global int *g_odata)
{
    // perform first level of reduction,
    // reading from global memory, writing to shared memory
    unsigned int tid = get_local_id(0);
    unsigned int i = get_group_id(0)*(get_local_size(0)*2) + get_local_id(0);

    sdata[tid] = (i < n) ? g_idata[i] : 0;
    if (i + get_local_size(0) < n) 
        sdata[tid] += g_idata[i+get_local_size(0)];  

    barrier(CLK_LOCAL_MEM_FENCE);

    // write result for this block to global mem 
    if (tid < 64) {
        sdata[tid] += sdata[tid + 64];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 32) {
        sdata[tid] += sdata[tid + 32];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 16) {
        sdata[tid] += sdata[tid + 16];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 8) {
        sdata[tid] += sdata[tid + 8];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 4) {
        sdata[tid] += sdata[tid + 4];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 2) {
        sdata[tid] += sdata[tid + 2];
        barrier(CLK_LOCAL_MEM_FENCE);
	}
	if (tid < 1) {
        sdata[tid] += sdata[tid + 1];
	}

    // write result for this block to global mem 
    if (tid == 0) g_odata[get_group_id(0)] = sdata[0];
}

__kernel void reduce_serial(__global int* buffer, __const int block, __const int length, __global int* result) {  
	int global_index = get_global_id(0) * block;
	int accumulator = 0;
	int upper_bound = (get_global_id(0) + 1) * block;
	if (upper_bound > length) upper_bound = length;
	while (global_index < upper_bound) {
		accumulator += buffer[global_index];
		global_index++;
	}
	result[get_group_id(0)] = accumulator;
}


__kernel void reduce_serial_int4(__global int4* buffer, __const int block, __const int length, __global int4* result) {  
	int global_index = get_global_id(0) * block;
	int4 accumulator = 0;
	int upper_bound = (get_global_id(0) + 1) * block;
	if (upper_bound > length) upper_bound = length;
	while (global_index < upper_bound) {
		accumulator += buffer[global_index];
		global_index++;
	}
	result[get_group_id(0)] = accumulator;
}
