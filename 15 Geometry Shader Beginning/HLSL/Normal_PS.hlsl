#include "Basic.fx"

float4 PS(VertexPosHWNormalColor pIn) : SV_TARGET
{
    return pIn.Color;
}