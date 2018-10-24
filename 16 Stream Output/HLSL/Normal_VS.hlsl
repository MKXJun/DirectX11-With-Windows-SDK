#include "Basic.hlsli"

VertexPosHWNormalColor VS(VertexPosNormalColor vIn)
{
    VertexPosHWNormalColor vOut;
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(vIn.PosL, 1.0f), gWorld);

    vOut.PosH = mul(posW, viewProj);
    vOut.PosW = posW.xyz;
    vOut.NormalW = mul(vIn.NormalL, (float3x3) gWorldInvTranspose);
    vOut.Color = vIn.Color;
    return vOut;
}