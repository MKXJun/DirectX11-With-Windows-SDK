#include "Basic.hlsli"

// ¶¥µã×ÅÉ«Æ÷(2D)
Vertex2DOut VS_2D(Vertex2DIn vIn)
{
    Vertex2DOut vOut;
    vOut.PosH = float4(vIn.Pos, 1.0f);
    vOut.Tex = vIn.Tex;
    return vOut;
}
