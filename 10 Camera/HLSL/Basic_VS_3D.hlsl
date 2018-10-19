#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷(3D)
VertexOut VS_3D(VertexIn pIn)
{
    VertexOut pOut;
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(pIn.PosL, 1.0f), gWorld);

    pOut.PosH = mul(posW, viewProj);
    pOut.PosW = posW.xyz;
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}