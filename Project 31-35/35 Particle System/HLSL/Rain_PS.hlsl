#include "Rain.hlsli"

float4 PS(GeoOut pIn) : SV_Target
{
    return g_TexArray.Sample(g_SamLinear, float3(pIn.Tex, 0.0f));
}
