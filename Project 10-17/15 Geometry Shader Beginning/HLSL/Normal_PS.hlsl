#include "Basic.hlsli"

float4 PS(VertexPosHWNormalColor pIn) : SV_TARGET
{
    return pIn.color;
}