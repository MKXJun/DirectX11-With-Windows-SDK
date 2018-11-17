#include "Sky.hlsli"

float4 PS(VertexPosHL pIn) : SV_Target
{
    return texCube.Sample(sam, pIn.PosL);
}
