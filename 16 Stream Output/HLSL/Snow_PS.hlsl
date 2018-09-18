#include "BasicObject.fx"

float4 PS(VertexPosHColor pIn) : SV_Target
{
    return pIn.Color;
}
