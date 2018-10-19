#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(VertexPosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    matrix viewProj = mul(gView, gProj);
    vector posW = mul(float4(pIn.PosL, 1.0f), gWorld);

    pOut.PosW = posW.xyz;
    pOut.PosH = mul(posW, viewProj);
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}