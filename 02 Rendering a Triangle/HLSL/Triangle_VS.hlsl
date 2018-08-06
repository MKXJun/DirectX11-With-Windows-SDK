#include "Triangle.fx"

// 顶点着色器
VertexOut VS(VertexIn pIn)
{
    VertexOut pOut;
    pOut.posH = float4(pIn.pos, 1.0f);
    pOut.color = pIn.color; // 这里alpha通道的值默认为1.0
    return pOut;
}