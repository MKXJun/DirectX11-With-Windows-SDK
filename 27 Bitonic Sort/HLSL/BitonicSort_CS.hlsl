#include "BitonicSort.hlsli"

#define BITONIC_BLOCK_SIZE 512

groupshared uint shared_data[BITONIC_BLOCK_SIZE];

[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void CS(uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex)
{
    // 写入共享数据
    shared_data[GI] = g_Data[DTid.x];
    GroupMemoryBarrierWithGroupSync();
    
    // 进行排序
    for (uint j = g_Level >> 1; j > 0; j >>= 1)
    {
        // 所处序列  当前索引  比较结果  取值结果
        //   递减      小索引    <=      (较大值)较大索引的值
        //   递减      大索引    <=      (较小值)较大索引的值
        //   递增      小索引    <=      (较小值)较小索引的值
        //   递增      大索引    <=      (较大值)较小索引的值
        //   递减      小索引    >       (较大值)较小索引的值
        //   递减      大索引    >       (较小值)较小索引的值
        //   递增      小索引    >       (较小值)较大索引的值 
        //   递增      大索引    >       (较大值)较大索引的值
        
        uint smallerIndex = GI & ~j;
        uint largerIndex = GI | j;
        bool isSmallerIndex = (GI == smallerIndex);
        bool isDescending = (bool) (g_DescendMask & DTid.x);
        uint result = ((shared_data[smallerIndex] <= shared_data[largerIndex]) == (isDescending == isSmallerIndex)) ?
            shared_data[largerIndex] : shared_data[smallerIndex];
        GroupMemoryBarrierWithGroupSync();

        shared_data[GI] = result;
        GroupMemoryBarrierWithGroupSync();
    }
    
    // 保存结果
    g_Data[DTid.x] = shared_data[GI];
}
