cbuffer CBSettings : register(b0)
{
    int g_BlurRadius;
    
    // 最多支持19个模糊权值
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
    float w11;
    float w12;
    float w13;
    float w14;
    float w15;
    float w16;
    float w17;
    float w18;
}

Texture2D g_Input : register(t0);
RWTexture2D<float4> g_Output : register(u0);

static const int g_MaxBlurRadius = 9;

#define N 256
#define CacheSize (N + 2 * g_MaxBlurRadius)
