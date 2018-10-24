#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷(3D)
VertexOut VS_3D(VertexIn vIn)
{
    VertexOut vOut;
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(vIn.PosL, 1.0f), gWorld);

    vOut.PosH = mul(posW, viewProj);
    vOut.PosW = posW.xyz;
    vOut.NormalW = mul(vIn.NormalL, (float3x3) gWorldInvTranspose);
    vOut.Tex = vIn.Tex;
    return vOut;
}