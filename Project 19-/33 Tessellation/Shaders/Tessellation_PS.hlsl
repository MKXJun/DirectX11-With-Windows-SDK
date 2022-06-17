#include "Tessellation.hlsli"

float4 PS(float4 posH : SV_position) : SV_Target
{
    return g_Color;
}
