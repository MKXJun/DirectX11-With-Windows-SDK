#ifndef SKYBOX_HLSL
#define SKYBOX_HLSL

uniform matrix g_ViewProj;

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texCoord : TEXCOORD;
};

//--------------------------------------------------------------------------------------
// 后处理, 天空盒等
// 使用天空盒几何体渲染
//--------------------------------------------------------------------------------------
TextureCube<float4> g_SkyboxTexture : register(t5);
Texture2D<float> g_DepthTexture : register(t6);
// 常规场景渲染的纹理
Texture2D<float4> g_LitTexture : register(t7);

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

SamplerState g_SamplerDiffuse : register(s0);

float4 SkyboxPS(SkyboxVSOut input) : SV_Target
{
    uint2 coords = input.posViewport.xy;

    float3 lit = float3(0.0f, 0.0f, 0.0f);
    float depth = g_DepthTexture[coords];
    if (depth >= 1.0f)
    {
        lit += g_SkyboxTexture.Sample(g_SamplerDiffuse, input.skyboxCoord).rgb;
    }
    else
    {
        lit += g_LitTexture[coords].rgb;
    }
    
    return float4(lit, 1.0f);
}


#endif // SKYBOX_HLSL
