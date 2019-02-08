#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTangentTex VS(VertexPosNormalTangentTex vIn)
{
    VertexPosHWNormalTangentTex vOut;
    
    matrix viewProj = mul(gView, gProj);
    vector posW = mul(float4(vIn.PosL, 1.0f), gWorld);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, viewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) gWorldInvTranspose);
    vOut.TangentW = mul(vIn.TangentL, gWorld);
    vOut.Tex = vIn.Tex;
    return vOut;
}