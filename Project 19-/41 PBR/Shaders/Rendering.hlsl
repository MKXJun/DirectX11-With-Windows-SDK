
#ifndef RENDERING_HLSL
#define RENDERING_HLSL

#include "PBR.hlsl"
#include "FullScreenTriangle.hlsl"
#include "ConstantBuffers.hlsl"
#include "CascadedShadow.hlsl"

SamplerState g_Sam : register(s0);

//--------------------------------------------------------------------------------------
// 几何阶段 
//--------------------------------------------------------------------------------------
struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
#if defined(USE_NORMAL_MAP) && defined(USE_TANGENT)
    float4 tangentL : TANGENT;
#endif
    float2 texCoord : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posV : POSITION;
    float3 normalW : NORMAL0;
    float3 normalV : NORMAL1;
#if defined(USE_NORMAL_MAP) && defined(USE_TANGENT)
    float4 tangentV : TANGENT;
#endif
    float2 texCoord : TEXCOORD0;
};

VertexOut GeometryVS(VertexPosNormalTex input)
{
    VertexOut output;
    
    output.posH = mul(float4(input.posL, 1.0f), g_WorldViewProj);
    output.posV = mul(float4(input.posL, 1.0f), g_WorldView).xyz;
    output.normalV = normalize(mul(input.normalL, (float3x3) g_WorldInvTransposeView));
    output.normalW = normalize(mul(output.normalV, (float3x3) g_InvView));
#if defined(USE_NORMAL_MAP) && defined(USE_TANGENT)
    output.tangentV = float4(normalize(mul(input.tangentL.xyz, (float3x3) g_WorldView)), input.tangentL.w);
#endif
    output.texCoord = input.texCoord;
    return output;
}


#endif // RENDERING_HLSL
