#include "Sky.hlsli"

float4 PS(VertexPosHL pIn) : SV_Target
{
    return gTexCube.Sample(gSam, pIn.PosL);
}
