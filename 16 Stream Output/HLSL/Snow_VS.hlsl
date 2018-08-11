#include "Basic.fx"

VertexPosHColor VS(VertexPosColor pIn)
{
    row_major matrix worldViewProj = mul(mul(gWorld, gView), gProj);
    VertexPosHColor pOut;
    pOut.Color = pIn.Color;
    pOut.PosH = mul(float4(pIn.PosL, 1.0f), worldViewProj);
    return pOut;
}
