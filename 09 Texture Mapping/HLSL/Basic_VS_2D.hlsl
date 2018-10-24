#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷(2D)
VertexOut VS_2D(VertexIn vIn)
{
    VertexOut vOut;
    vOut.PosH = float4(vIn.PosL, 1.0f);
    vOut.PosW = float3(0.0f, 0.0f, 0.0f);
    vOut.NormalW = vIn.NormalL;
    vOut.Tex = vIn.Tex;
    return vOut;
}