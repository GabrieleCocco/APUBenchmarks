__kernel void saxpy(const __global float * x, __global float * y, const float a, const int offset, const int size) 
{
	uint guid = get_global_id(0) + offset;
	if(guid < size)
		y[guid] = a * x[guid] + y[guid];
}

__kernel void saxpy2(const __global float2 * x, __global float2 * y, const float a, const int offset, const int size) 
{
	uint guid = get_global_id(0) + offset;
	if(guid < size)
		y[guid] = a * x[guid] + y[guid];
}

__kernel void saxpy4(const __global float4 * x, __global float4 * y, const float a, const int offset, const int size) 
{
	uint guid = get_global_id(0) + offset;
	if(guid < size)
		y[guid] = a * x[guid] + y[guid];
}

__kernel void saxpy8(const __global float8 * x, __global float8 * y, const float a, const int offset, const int size) 
{
	uint guid = get_global_id(0) + offset;
	if(guid < size)
		y[guid] = a * x[guid] + y[guid];
}
