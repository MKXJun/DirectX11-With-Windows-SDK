#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷(2D)
Vertex2DOut VS_2D(Vertex2DIn pIn)
{
    Vertex2DOut pOut;
    pOut.PosH = float4(pIn.Pos, 1.0f);
    pOut.Tex = pIn.Tex;
    return pOut;
}
