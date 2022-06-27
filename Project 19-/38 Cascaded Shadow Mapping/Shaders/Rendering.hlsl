
#ifndef RENDERING_HLSL
#define RENDERING_HLSL

#include "FullScreenTriangle.hlsl"
#include "ConstantBuffers.hlsl"
#include "CascadedShadow.hlsl"

//--------------------------------------------------------------------------------------
// 几何阶段 
//--------------------------------------------------------------------------------------
Texture2D g_DiffuseMap : register(t0);
SamplerState g_Sam : register(s0);

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 texCoord : TEXCOORD0;
    float4 shadowPosV : TEXCOORD1;
    float depthV : TEXCOORD2;
};

VertexOut GeometryVS(VertexPosNormalTex input)
{
    VertexOut output;

    output.posH = mul(float4(input.posL, 1.0f), g_WorldViewProj);
    output.posW = mul(float4(input.posL, 1.0f), g_World).xyz;
    output.normalW = mul(float4(input.normalL, 0.0f), g_WorldInvTranspose).xyz;
    output.texCoord = input.texCoord;
    output.shadowPosV = mul(float4(output.posW, 1.0f), g_ShadowView);
    output.depthV = mul(float4(input.posL, 1.0f), g_WorldView).z;
    return output;
}


//--------------------------------------------------------------------------------------
// 光照阶段 
//--------------------------------------------------------------------------------------
float4 ForwardPS(VertexOut input) : SV_Target
{
    int cascadeIndex = 0;
    int nextCascadeIndex = 0;
    float blendAmount = 0.0f;
    float percentLit = CalculateCascadedShadow(input.shadowPosV, input.depthV,
        cascadeIndex, nextCascadeIndex, blendAmount);
    
    float4 diffuse = g_DiffuseMap.Sample(g_Sam, input.texCoord);
    
    float4 visualizeCascadeColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (g_VisualizeCascades)
        visualizeCascadeColor = GetCascadeColorMultipler(cascadeIndex, nextCascadeIndex, saturate(blendAmount));
    
    // 灯光硬编码
    float3 lightDir[4] =
    {
        float3(-1.0f, 1.0f, -1.0f),
        float3(1.0f, 1.0f, -1.0f),
        float3(0.0f, -1.0f, 0.0f),
        float3(1.0f, 1.0f, 1.0f)
    };
    // 类似环境光的照明
    float lighting = saturate(dot(lightDir[0], input.normalW)) * 0.05f +
                     saturate(dot(lightDir[1], input.normalW)) * 0.05f +
                     saturate(dot(lightDir[2], input.normalW)) * 0.05f +
                     saturate(dot(lightDir[3], input.normalW)) * 0.05f;
    
    float shadowLighting = lighting * 0.5f;
    lighting += saturate(dot(-g_LightDir, input.normalW));
    lighting = lerp(shadowLighting, lighting, percentLit);
    return lighting * visualizeCascadeColor * diffuse;
}


#endif // RENDERING_HLSL
