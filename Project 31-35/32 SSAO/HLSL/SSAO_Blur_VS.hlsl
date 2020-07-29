#include "SSAO.hlsli"

// 绘制SSAO图的顶点着色器
VertexPosHTex VS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;
    
    // 已经在NDC空间
    vOut.PosH = float4(vIn.PosL, 1.0f);
    
    vOut.Tex = vIn.Tex;
    
    return vOut;
}
