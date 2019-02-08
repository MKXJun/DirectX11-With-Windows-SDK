#include "ScreenFade.hlsli"

// 顶点着色器
VertexPosHTex VS(VertexPosTex vIn)
{
    VertexPosHTex vOut;
    
    vOut.PosH = mul(float4(vIn.PosL, 1.0f), gWorldViewProj);
    vOut.Tex = vIn.Tex;
    return vOut;
}