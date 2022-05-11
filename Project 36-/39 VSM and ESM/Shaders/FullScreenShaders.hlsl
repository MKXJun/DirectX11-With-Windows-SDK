
#ifndef FULL_SCREEN_SHADERS_HLSL
#define FULL_SCREEN_SHADERS_HLSL


#include "FullScreenTriangle.hlsl"

#ifndef BLUR_KERNEL_SIZE
#define BLUR_KERNEL_SIZE 3
#endif

static const int BLUR_KERNEL_BEGIN = BLUR_KERNEL_SIZE / -2;
static const int BLUR_KERNEL_END = BLUR_KERNEL_SIZE / 2 + 1;
static const float FLOAT_BLUR_KERNEL_SIZE = (float)BLUR_KERNEL_SIZE;

cbuffer CB : register(b0)
{
    float4 g_BlurWeightsArray[4];
    static float g_BlurWeights[16] = (float[16]) g_BlurWeightsArray;
}

Texture2D<float2> g_TextureShadow : register(t0);
SamplerState g_SamplerPointClamp : register(s0);

float2 PSBlurX(float4 posH : SV_Position,
               float2 texcoord : TEXCOORD) : SV_Target
{
    float2 depths = 0.0f;
    [unroll]
    for (int x = BLUR_KERNEL_BEGIN; x < BLUR_KERNEL_END; ++x)
    {
        depths += g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(x, 0));
    }
    depths /= FLOAT_BLUR_KERNEL_SIZE;
    return depths;
}

float2 PSBlurY(float4 posH : SV_Position,
               float2 texcoord : TEXCOORD) : SV_Target
{
    float2 depths = 0.0f;
    [unroll]
    for (int y = BLUR_KERNEL_BEGIN; y < BLUR_KERNEL_END; ++y)
    {
        depths += g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(0, y));
    }
    depths /= FLOAT_BLUR_KERNEL_SIZE;
    return depths;
}

float PSLogGaussianBlur(float4 posH : SV_Position,
                         float2 texcoord : TEXCOORD) : SV_Target
{
    float cd0 = g_TextureShadow.Sample(g_SamplerPointClamp, texcoord);
    float sum = g_BlurWeights[FLOAT_BLUR_KERNEL_SIZE / 2] * g_BlurWeights[FLOAT_BLUR_KERNEL_SIZE / 2];
    [unroll]
    for (int i = BLUR_KERNEL_BEGIN; i < BLUR_KERNEL_END; ++i)
    {
        for (int j = BLUR_KERNEL_BEGIN; j < BLUR_KERNEL_END; ++j)
        {
            float cdk = g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(i, j)) * (float) (i != 0 || j != 0);
            sum += g_BlurWeights[i - BLUR_KERNEL_BEGIN] * g_BlurWeights[j - BLUR_KERNEL_BEGIN] * exp(cdk - cd0);
        }
    }
    sum = log(sum) + cd0;
    sum = isinf(sum) ? 88.0f : sum;
    return sum;
}

#endif // FULL_SCREEN_SHADERS_HLSL
