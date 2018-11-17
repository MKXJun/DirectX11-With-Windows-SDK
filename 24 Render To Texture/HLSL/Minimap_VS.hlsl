#include "Minimap.hlsli"

// ¶¥µã×ÅÉ«Æ÷(2D)
VertexPosHTex VS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;
    vOut.PosH = float4(vIn.PosL, 1.0f);
    vOut.Tex = vIn.Tex;
    return vOut;
}