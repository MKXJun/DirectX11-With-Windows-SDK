#include "Sky.hlsli"

float4 PS(VertexPosHL pIn) : SV_Target
{
    return g_TexCube.Sample(g_Sam, pIn.PosL);
}
