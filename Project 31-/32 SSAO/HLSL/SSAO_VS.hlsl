#include "SSAO.hlsli"

// 绘制SSAO图的顶点着色器
VertexOut VS(VertexIn vIn)
{
    VertexOut vOut;
    
    // 已经在NDC空间
    vOut.PosH = float4(vIn.PosL, 1.0f);
    
    // 我们用它的x分量来索引视锥体远平面的顶点数组
    vOut.ToFarPlane = g_FrustumCorners[vIn.ToFarPlaneIndex.x].xyz;
    
    vOut.Tex = vIn.Tex;
    
    return vOut;
}
