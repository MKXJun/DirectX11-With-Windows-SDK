
#ifndef SKYBOX_HLSL
#define SKYBOX_HLSL

#include "GBuffer.hlsl"

#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 1
#endif

//--------------------------------------------------------------------------------------
// 后处理, 天空盒等
// 使用天空盒几何体渲染
//--------------------------------------------------------------------------------------
TextureCube<float4> g_SkyboxTexture : register(t5);
Texture2DMS<float, MSAA_SAMPLES> g_DepthTexture : register(t6);

// 常规多重采样的场景渲染的纹理
Texture2DMS<float4, MSAA_SAMPLES> g_LitTexture : register(t7);

struct SkyboxVSOut
{
    float4 posViewport : SV_Position;
    float3 skyboxCoord : skyboxCoord;
};

SkyboxVSOut SkyboxVS(VertexPosNormalTex input)
{
    SkyboxVSOut output;
    
    // 注意：不要移动天空盒并确保深度值为1(避免裁剪)
    output.posViewport = mul(float4(input.posL, 0.0f), g_ViewProj).xyww;
    output.skyboxCoord = input.posL;
    
    return output;
}

float4 SkyboxPS(SkyboxVSOut input) : SV_Target
{
    uint2 coords = input.posViewport.xy;

    float3 lit = float3(0.0f, 0.0f, 0.0f);
    float skyboxSamples = 0.0f;
#if MSAA_SAMPLES <= 1
    [unroll]
#endif
    for (unsigned int sampleIndex = 0; sampleIndex < MSAA_SAMPLES; ++sampleIndex)
    {
        float depth = g_DepthTexture.Load(coords, sampleIndex);

        // 检查天空盒的状态（注意：反向Z!）  
        if (depth <= 0.0f && !g_VisualizeLightCount)
        {
            ++skyboxSamples;
        }
        else
        {
            lit += g_LitTexture.Load(coords, sampleIndex).xyz;
        }
    }

    // 如果这里没有场景渲染，则渲染天空盒
    [branch]
    if (skyboxSamples > 0)
    {
        float3 skybox = g_SkyboxTexture.Sample(g_Sam, input.skyboxCoord).xyz;
        lit += skyboxSamples * skybox;
    }
    
    // Resolve 多重采样(简单盒型滤波)
    return float4(lit * rcp((float)MSAA_SAMPLES), 1.0f);
}


#endif // SKYBOX_HLSL
