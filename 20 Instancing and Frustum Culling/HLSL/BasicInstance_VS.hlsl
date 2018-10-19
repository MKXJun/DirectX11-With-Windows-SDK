#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(InstancePosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    matrix viewProj = mul(gView, gProj);
    vector posW = mul(float4(pIn.PosL, 1.0f), pIn.World);

    pOut.PosW = posW.xyz;
    pOut.PosH = mul(posW, viewProj);
    pOut.NormalW = mul(pIn.NormalL, (float3x3) pIn.WorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}