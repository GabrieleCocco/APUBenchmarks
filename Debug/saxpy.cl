__kernel void saxpy(__global float * a, __global float * b, __global float * c) 
{
	uint guid = get_global_id(0);
	c[guid] = a[guid] + b[guid];
}