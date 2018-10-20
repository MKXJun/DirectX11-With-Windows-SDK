#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷(2D)
VertexOut VS_2D(VertexIn pIn)
{
    VertexOut pOut;
    pOut.PosH = float4(pIn.PosL, 1.0f);
    pOut.PosW = float3(0.0f, 0.0f, 0.0f);
    pOut.NormalW = pIn.NormalL;
    pOut.Tex = pIn.Tex;
    return pOut;
}