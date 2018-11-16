
__kernel void vector_add(__global const float *a,
						__global const float *b,
						__global float *result,
                        const int num)
{
    int i;
    int gid = get_global_id(0);
    int grid_size = get_global_size(0);
    for(i=gid;i<num;i+=grid_size){
        result[i] = a[i] + b[i];
    }
}
