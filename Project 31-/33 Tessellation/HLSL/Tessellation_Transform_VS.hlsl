#include "Tessellation.hlsli"

float4 VS(float3 PosL : POSITION) : SV_POSITION
{
    return mul(float4(PosL, 1.0f), g_WorldViewProj);
}
