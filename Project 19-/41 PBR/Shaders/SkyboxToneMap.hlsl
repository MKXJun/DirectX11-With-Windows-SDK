
#ifndef SKYBOX_TONE_MAP_HLSL
#define SKYBOX_TONE_MAP_HLSL

#include "Constants.hlsl"

cbuffer CB0 : register(b0)
{
    matrix g_ViewProj;
};

cbuffer CB1 : register(b1)
{
    matrix g_ViewProjs[6];
}

//--------------------------------------------------------------------------------------
// 后处理, 天空盒等
// 使用天空盒几何体渲染
//--------------------------------------------------------------------------------------
TextureCube<float4> g_SkyboxTexture : register(t0);
Texture2D<float> g_DepthTexture : register(t1);

// 常规多重采样的场景渲染的纹理
Texture2D<float4> g_LitTexture : register(t2);
// 等距柱状投影图
Texture2D<float3> g_EquirectangularTexture : register(t3);

SamplerState g_SamLinearWrap : register(s0);

struct SkyboxVSOut
{
    float4 posViewport : SV_Position;
    float3 skyboxCoord : skyboxCoord;
};

static uint s_CubeIndices[] = { 
    0, 1, 2, 1, 3, 2,
    6, 7, 4, 7, 5, 4,
    4, 5, 0, 5, 1, 0,
    2, 3, 6, 3, 7, 6,
    4, 0, 6, 0, 2, 6,
    1, 5, 3, 5, 7, 3
};

// Reinhard tone operator
// TM_Reinhard(x, 0.5) == TM_Reinhard(x * 2.0, 1.0)
float3 TM_Reinhard(float3 hdr, float k)
{
    return hdr / (hdr + k);
}

float3 TM_Standard(float3 hdr)
{
    return TM_Reinhard(hdr * sqrt(hdr), sqrt(4.0f / 27.0f));
}

float3 TM_ACES_Coarse(float3 hdr)
{
    static const float a = 2.51f;
    static const float b = 0.03f;
    static const float c = 2.43f;
    static const float d = 0.59f;
    static const float e = 0.14f;
    return saturate((hdr * (a * hdr + b)) / (hdr * (c * hdr + d) + e));
}

float3 TM_ACES(float3 hdr)
{
    static float3x3 aces_input_mat = float3x3(
        0.59719f, 0.35458f, 0.04823f,
        0.07600f, 0.90834f, 0.01566f,
        0.02840f, 0.13383f, 0.83777f);
    
    static float3x3 aces_output_mat = float3x3(
        1.60475f, -0.53108f, -0.07367f,
        -0.10208f, 1.10813f, -0.00605f,
        -0.00327f, -0.07276f, 1.07602f);
    
    float3 res = mul(aces_input_mat, hdr);
    float3 a = res * (res + 0.0245786f) - 0.000090537f;
    float3 b = res * (0.983729f * res + 0.4329510f) + 0.238081f;
    return mul(aces_output_mat, a / b);
}

SkyboxVSOut SkyboxToneMapVS(uint vertexID : SV_VertexID)
{
    SkyboxVSOut output;
    uint idx = s_CubeIndices[vertexID];
    float3 posW = float3(idx / 2 % 2, idx % 2, idx / 4) * 2.0f - 1.0f;
    // 注意：不要移动天空盒并确保深度值为1(避免裁剪)
    output.posViewport = mul(float4(posW, 0.0f), g_ViewProj).xyww;
    output.skyboxCoord = posW;
    
    return output;
}

float4 SkyboxToneMapCubePS(SkyboxVSOut input) : SV_Target
{
    uint2 coords = input.posViewport.xy;

    float3 lit = float3(0.0f, 0.0f, 0.0f);
    float skyboxSamples = 0.0f;
    float depth = g_DepthTexture[coords];

    // 注意：反向Z
    [branch]
    if (depth <= 0.0f)
        lit = g_SkyboxTexture.Sample(g_SamLinearWrap, input.skyboxCoord).xyz;
    else
    {
        uint texWidth, texHeight;
        g_LitTexture.GetDimensions(texWidth, texHeight);
        lit = g_LitTexture.SampleLevel(g_SamLinearWrap, input.posViewport.xy / float2(texWidth, texHeight), 0.0f).rgb;

    } 
    
    // Tone Mapping
#if defined(TONEMAP_ACES)
    return float4(TM_ACES(lit), 1.0f);
#elif defined(TONEMAP_ACES_COARSE)
    return float4(TM_ACES_Coarse(lit), 1.0f);
#elif defined(TONEMAP_STANDARD)
    return float4(TM_Standard(lit), 1.0f);
#else
    return float4(TM_Reinhard(lit, 1.0f), 1.0f);
#endif
    
}

#endif // SKYBOX_TONE_MAP_HLSL
