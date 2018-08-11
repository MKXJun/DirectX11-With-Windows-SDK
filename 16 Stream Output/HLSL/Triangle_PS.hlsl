#include "Basic.fx"

float4 PS(VertexPosHColor pIn) : SV_Target
{
    return pIn.Color;
}
