#include "Tessellation.hlsli"

float4 PS(float4 PosH : SV_Position) : SV_Target
{
    return g_Color;
}
