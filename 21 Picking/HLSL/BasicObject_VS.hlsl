#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(VertexPosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    row_major matrix viewProj = mul(gView, gProj);

    pOut.PosW = mul(float4(pIn.PosL, 1.0f), gWorld).xyz;
    pOut.PosH = mul(float4(pOut.PosW, 1.0f), viewProj);
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}