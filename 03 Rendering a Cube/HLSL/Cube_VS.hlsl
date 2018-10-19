#include "Cube.fx"

VertexOut VS(VertexIn pIn)
{
    VertexOut pOut;
    pOut.posH = mul(float4(pIn.posL, 1.0f), gWorld);  // mul 才是矩阵乘法, 运算符*要求操作对象为
    pOut.posH = mul(pOut.posH, gView);               // 行列数相等的两个矩阵，结果为
    pOut.posH = mul(pOut.posH, gProj);               // Cij = Aij * Bij
    pOut.color = pIn.color;                         // 这里alpha通道的值默认为1.0
    return pOut;
}