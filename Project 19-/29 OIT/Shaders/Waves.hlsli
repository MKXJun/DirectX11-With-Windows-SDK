// 用于更新模拟
cbuffer cbUpdateSettings : register(b0)
{
    float g_WaveConstant0;
    float g_WaveConstant1;
    float g_WaveConstant2;
    float g_DisturbMagnitude;
    
    int2 g_DisturbIndex;
    float2 g_Pad;
}

RWTexture2D<float> g_PrevSolInput : register(u0);
RWTexture2D<float> g_CurrSolInput : register(u1);
RWTexture2D<float> g_Output : register(u2);
