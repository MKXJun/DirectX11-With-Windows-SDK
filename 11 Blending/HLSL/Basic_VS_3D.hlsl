#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷(3D)
VertexOut VS_3D(VertexIn pIn)
{
    VertexOut pOut;
    row_major matrix worldViewProj = mul(mul(gWorld, gView), gProj);
    pOut.PosH = mul(float4(pIn.Pos, 1.0f), worldViewProj);
    pOut.PosW = mul(float4(pIn.Pos, 1.0f), gWorld).xyz;
    pOut.NormalW = mul(pIn.Normal, (float3x3) gWorldInvTranspose);
    pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
    return pOut;
}
