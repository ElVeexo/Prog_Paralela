__kernel void kernel_grafos(__global const bool* matriz, 
                            __global bool* resultado, 
                            const int N,
                            __local bool* scratch_A) 
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);
    
    /barrier(CLK_LOCAL_MEM_FENCE);
}