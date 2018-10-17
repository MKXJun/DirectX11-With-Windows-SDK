#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷
VertexPosHWNormalTex VS(InstancePosNormalTex pIn)
{
    VertexPosHWNormalTex pOut;
    
    row_major matrix viewProj = mul(gView, gProj);
    
    pOut.PosW = mul(float4(pIn.PosL, 1.0f), pIn.World).xyz;
    pOut.PosH = mul(float4(pOut.PosW, 1.0f), viewProj);
    pOut.NormalW = mul(pIn.NormalL, (float3x3) pIn.WorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}