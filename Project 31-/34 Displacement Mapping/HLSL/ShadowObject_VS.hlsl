#include "Shadow.hlsli"

VertexPosHTex VS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;

    vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_WorldViewProj);
    vOut.Tex = vIn.Tex;

    return vOut;
}
