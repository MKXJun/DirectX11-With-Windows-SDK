#include "Basic.hlsli"

VertexPosHWNormalColor VS(VertexPosNormalColor vIn)
{
    VertexPosHWNormalColor vOut;
    matrix viewProj = mul(g_View, g_Proj);
    float4 posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posH = mul(posW, viewProj);
    vOut.posW = posW.xyz;
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.color = vIn.color;
    return vOut;
}

