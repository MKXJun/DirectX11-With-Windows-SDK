#include "Basic.fx"

// ¶¥µã×ÅÉ«Æ÷(2D)
Vertex2DOut VS_2D(Vertex2DIn pIn)
{
    Vertex2DOut pOut;
    pOut.PosH = float4(pIn.Pos, 1.0f);
    pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
    return pOut;
}
