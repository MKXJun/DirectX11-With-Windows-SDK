cbuffer CBSettings : register(b0)
{
    int g_BlurRadius;
    
    // 最多支持31个模糊权值
    float4 g_Weights[8];
    // 不位于常量缓冲区
    static float s_Weights[32] = (float[32]) g_Weights;
}

Texture2D g_Input : register(t0);
Texture2D g_EdgeInput : register(t1);

RWTexture2D<float4> g_Output : register(u0);

SamplerState g_SamLinearWrap : register(s0); // 线性过滤+Wrap采样器
SamplerState g_SamPointClamp : register(s1); // 点过滤+Clamp采样器


#define MAX_BLUR_RADIUS 15
#define N 256
#define CacheSize (N + 2 * MAX_BLUR_RADIUS)

