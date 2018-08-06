#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷(2D)
VertexOut VS_2D(VertexIn pIn)
{
    VertexOut pOut;
    pOut.PosH = float4(pIn.Pos, 1.0f);
    pOut.PosW = float3(0.0f, 0.0f, 0.0f);
    pOut.NormalW = pIn.Normal;
    pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
    return pOut;
}