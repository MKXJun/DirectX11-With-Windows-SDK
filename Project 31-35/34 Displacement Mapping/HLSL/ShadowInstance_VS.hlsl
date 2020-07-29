#include "Shadow.hlsli"

VertexPosHTex VS(InstancePosNormalTex vIn)
{
    VertexPosHTex vOut;
    float4 posW = mul(float4(vIn.PosL, 1.0f), vIn.World);
    vOut.PosH = mul(posW, g_ViewProj);
    vOut.Tex = vIn.Tex;

    return vOut;
}
