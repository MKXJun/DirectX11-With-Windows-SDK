#ifndef SHADOW_HLSL
#define SHADOW_HLSL

#include "FullScreenTriangle.hlsl"

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texCoord : TEXCOORD;
};


cbuffer CBTransform : register(b0)
{
    matrix g_WorldViewProj;
}

Texture2D g_TextureShadow : register(t0);
SamplerState g_SamplerPointClamp : register(s0);


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

float4 DebugPS(float4 posH : SV_Position,
               float2 texCoord : TEXCOORD) : SV_Target
{
    uint2 coords = uint2(posH.xy);
    float depth = g_TextureShadow[coords];
    return float4(depth.rrr, 1.0f);
}

#endif
