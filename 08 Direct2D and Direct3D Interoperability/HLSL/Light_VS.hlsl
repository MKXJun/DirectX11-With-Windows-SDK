#include "Light.hlsli"

// 顶点着色器
VertexOut VS(VertexIn pIn)
{
    VertexOut pOut;
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(pIn.PosL, 1.0f), gWorld);

    pOut.PosH = mul(posW, viewProj);
    pOut.PosW = posW.xyz;
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Color = pIn.Color; // 这里alpha通道的值默认为1.0
    return pOut;
}