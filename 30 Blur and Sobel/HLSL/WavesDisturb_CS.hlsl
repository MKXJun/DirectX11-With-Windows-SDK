#include "Waves.hlsli"

[numthreads(1, 1, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    // 我们不需要进行边界检验，因为:
    // --读取超出边界的区域结果为0，和我们对边界处理的需求一致
    // --对超出边界的区域写入并不会执行
    uint x = g_DisturbIndex.x;
    uint y = g_DisturbIndex.y;
    
    float halfMag = 0.5f * g_DisturbMagnitude;
    
    // RW型资源允许读写，所以+=是允许的
    g_Output[uint2(x, y)] += g_DisturbMagnitude;
    g_Output[uint2(x + 1, y)] += halfMag;
    g_Output[uint2(x - 1, y)] += halfMag;
    g_Output[uint2(x, y + 1)] += halfMag;
    g_Output[uint2(x, y - 1)] += halfMag;
}