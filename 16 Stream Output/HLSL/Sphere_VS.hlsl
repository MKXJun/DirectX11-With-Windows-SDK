#include "BasicObject.fx"

VertexPosHWNormalColor VS(VertexPosNormalColor pIn)
{
    VertexPosHWNormalColor pOut;
    row_major matrix viewProj = mul(gView, gProj);
    pOut.PosW = mul(float4(pIn.PosL, 1.0f), gWorld).xyz;
    pOut.PosH = mul(float4(pOut.PosW, 1.0f), viewProj);
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Color = pIn.Color;
    return pOut;
}