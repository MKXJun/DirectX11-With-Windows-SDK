#include "Basic.hlsli"

VertexPosHWNormalColor VS(VertexPosNormalColor vIn)
{
    VertexPosHWNormalColor vOut;
    matrix viewProj = mul(g_View, g_Proj);
    float4 posW = mul(float4(vIn.PosL, 1.0f), g_World);

    vOut.PosH = mul(posW, viewProj);
    vOut.PosW = posW.xyz;
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.Color = vIn.Color;
    return vOut;
}