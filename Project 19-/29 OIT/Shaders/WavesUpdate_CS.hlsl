#include "Waves.hlsli"

[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    // 我们不需要进行边界检验，因为:
    // --读取超出边界的区域结果为0，和我们对边界处理的需求一致
    // --对超出边界的区域写入并不会执行
    uint x = DTid.x;
    uint y = DTid.y;
    
    g_Output[uint2(x, y)] =
        g_WaveConstant0 * g_PrevSolInput[uint2(x, y)].x +
        g_WaveConstant1 * g_CurrSolInput[uint2(x, y)].x +
        g_WaveConstant2 * (
            g_CurrSolInput[uint2(x, y + 1)].x +
            g_CurrSolInput[uint2(x, y - 1)].x +
            g_CurrSolInput[uint2(x + 1, y)].x +
            g_CurrSolInput[uint2(x - 1, y)].x);
    
}
