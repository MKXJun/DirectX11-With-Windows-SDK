#ifndef SHADOW_HLSL
#define SHADOW_HLSL

#include "FullScreenTriangle.hlsl"

#ifndef BLUR_KERNEL_SIZE
#define BLUR_KERNEL_SIZE 3
#endif

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texCoord : TEXCOORD;
};


static const int BLUR_KERNEL_BEGIN = BLUR_KERNEL_SIZE / -2;
static const int BLUR_KERNEL_END = BLUR_KERNEL_SIZE / 2 + 1;
static const float FLOAT_BLUR_KERNEL_SIZE = (float) BLUR_KERNEL_SIZE;

cbuffer CBTransform : register(b0)
{
    matrix g_WorldViewProj;
    float2 g_EvsmExponents;
    int    g_16BitShadow;
}

cbuffer CBBlur : register(b1)
{
    float4 g_BlurWeightsArray[4];
    static float g_BlurWeights[16] = (float[16]) g_BlurWeightsArray;
}


Texture2D g_TextureShadow : register(t0);
SamplerState g_SamplerPointClamp : register(s0);


float2 GetEVSMExponents(in float positiveExponent, in float negativeExponent, in int is16BitFormat)
{
    const float maxExponent = (is16BitFormat ? 5.54f : 42.0f);

    float2 exponents = float2(positiveExponent, negativeExponent);

    // 限制指数范围防止出现溢出
    return min(exponents, maxExponent);
}

// 输入的depth需要在[0, 1]的范围
float2 ApplyEvsmExponents(float depth, float2 exponents)
{
    depth = 2.0f * depth - 1.0f;
    float2 expDepth;
    expDepth.x = exp(exponents.x * depth);
    expDepth.y = -exp(-exponents.y * depth);
    return expDepth;
}

//
// ShadowMap
//

void ShadowVS(VertexPosNormalTex vIn,
              out float4 posH : SV_Position,
              out float2 texCoord : TEXCOORD)
{
    posH = mul(float4(vIn.posL, 1.0f), g_WorldViewProj);
    texCoord = vIn.texCoord;
}


float ShadowPS(float4 posH : SV_Position,
               float2 texCoord : TEXCOORD) : SV_Target
{
    return posH.z;
}

float2 VarianceShadowPS(float4 posH : SV_Position,
                        float2 texCoord : TEXCOORD) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    
    float2 depth;
    depth.x = g_TextureShadow[coords];
    depth.y = depth.x * depth.x;
    return depth;
}

float ExponentialShadowPS(float4 posH : SV_Position,
                          float2 texCoord : TEXCOORD, 
                          uniform float c) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    return c * g_TextureShadow[coords];
}

float2 EVSM2CompPS(float4 posH : SV_Position,
                   float2 texCoord : TEXCOORD) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    float2 exponents = GetEVSMExponents(g_EvsmExponents.x, g_EvsmExponents.y, g_16BitShadow);
    float2 depth = ApplyEvsmExponents(g_TextureShadow[coords].x, exponents);
    float2 outDepth = float2(depth.x, depth.x * depth.x);
    return outDepth;
}

float4 EVSM4CompPS(float4 posH : SV_Position,
                   float2 texCoord : TEXCOORD) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    float2 depth = ApplyEvsmExponents(g_TextureShadow[coords].x, g_EvsmExponents);
    float4 outDepth = float4(depth, depth * depth).xzyw;
    return outDepth;
}

float4 DebugPS(float4 posH : SV_Position,
               float2 texCoord : TEXCOORD) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    float depth = g_TextureShadow[coords];
    return float4(depth.rrr, 1.0f);
}

float4 GaussianBlurXPS(float4 posH : SV_Position,
                         float2 texcoord : TEXCOORD) : SV_Target
{
    float4 depths = 0.0f;
    [unroll]
    for (int x = BLUR_KERNEL_BEGIN; x < BLUR_KERNEL_END; ++x)
    {
        depths += g_BlurWeights[x - BLUR_KERNEL_BEGIN] * g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(x, 0));
    }
    return depths;
}

float4 GaussianBlurYPS(float4 posH : SV_Position,
                       float2 texcoord : TEXCOORD) : SV_Target
{
    float4 depths = 0.0f;
    [unroll]
    for (int y = BLUR_KERNEL_BEGIN; y < BLUR_KERNEL_END; ++y)
    {
        depths += g_BlurWeights[y - BLUR_KERNEL_BEGIN] * g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(0, y));
    }
    return depths;
}

float LogGaussianBlurPS(float4 posH : SV_Position,
                        float2 texcoord : TEXCOORD) : SV_Target
{
    float cd0 = g_TextureShadow.Sample(g_SamplerPointClamp, texcoord);
    float sum = g_BlurWeights[FLOAT_BLUR_KERNEL_SIZE / 2] * g_BlurWeights[FLOAT_BLUR_KERNEL_SIZE / 2];
    [unroll]
    for (int i = BLUR_KERNEL_BEGIN; i < BLUR_KERNEL_END; ++i)
    {
        for (int j = BLUR_KERNEL_BEGIN; j < BLUR_KERNEL_END; ++j)
        {
            float cdk = g_TextureShadow.Sample(g_SamplerPointClamp, texcoord, int2(i, j)).x * (float) (i != 0 || j != 0);
            sum += g_BlurWeights[i - BLUR_KERNEL_BEGIN] * g_BlurWeights[j - BLUR_KERNEL_BEGIN] * exp(cdk - cd0);
        }
    }
    sum = log(sum) + cd0;
    sum = isinf(sum) ? 84.0f : sum;  // 防止溢出
    return sum;
}

#endif
