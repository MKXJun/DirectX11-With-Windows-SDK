#include "Tessellation.hlsli"

float4 VS(float3 posL : POSITION) : SV_POSITION
{
    return mul(float4(posL, 1.0f), g_WorldViewProj);
}
