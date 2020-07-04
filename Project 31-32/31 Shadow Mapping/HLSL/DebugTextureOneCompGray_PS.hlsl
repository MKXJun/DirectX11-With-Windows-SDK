#include "DebugTexture.hlsli"

float4 PS(VertexPosHTex pIn, uniform int index) : SV_Target
{
    float comp[4] = (float[4])g_DiffuseMap.Sample(g_Sam, pIn.Tex);
    // 以灰度的形式呈现
    return float4(comp[index].rrr, 1.0f);
}
