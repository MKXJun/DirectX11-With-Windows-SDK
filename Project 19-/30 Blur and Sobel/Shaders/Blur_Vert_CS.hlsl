#include "PostProcess.hlsli"

groupshared float4 g_Cache[CacheSize];

[numthreads(1, N, 1)]
void CS(int3 GTid : SV_GroupThreadID,
    int3 DTid : SV_DispatchThreadID)
{
    // 通过填写本地线程存储区来减少带宽的负载。若要对N个像素进行模糊处理，根据模糊半径，
    // 我们需要加载N + 2 * BlurRadius个像素
    
    // 此线程组运行着N个线程。为了获取额外的2*BlurRadius个像素，就需要有2*BlurRadius个
    // 线程都多采集一个像素数据
    if (GTid.y < g_BlurRadius)
    {
        // 对于图像上侧边界存在越界采样的情况进行钳位(Clamp)操作
        int y = max(DTid.y - g_BlurRadius, 0);
        g_Cache[GTid.y] = g_Input[int2(DTid.x, y)];
    }
    
    uint texWidth, texHeight;
    g_Input.GetDimensions(texWidth, texHeight);
    
    if (GTid.y >= N - g_BlurRadius)
    {
        // 对于图像下侧边界存在越界采样的情况进行钳位(Clamp)操作
        int y = min(DTid.y + g_BlurRadius, texHeight - 1);
        g_Cache[GTid.y + 2 * g_BlurRadius] = g_Input[int2(DTid.x, y)];
    }
    
    // 将数据写入Cache的对应位置
    // 针对图形边界处的越界采样情况进行钳位处理
    g_Cache[GTid.y + g_BlurRadius] = g_Input[min(DTid.xy, float2(texWidth, texHeight) - 1)];
    
    // 等待所有线程完成任务
    GroupMemoryBarrierWithGroupSync();
    
    // 开始对每个像素进行混合
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = -g_BlurRadius; i <= g_BlurRadius; ++i)
    {
        int k = GTid.y + g_BlurRadius + i;
        
        blurColor += s_Weights[i + g_BlurRadius] * g_Cache[k];
    }
    
    g_Output[DTid.xy] = blurColor;
}
