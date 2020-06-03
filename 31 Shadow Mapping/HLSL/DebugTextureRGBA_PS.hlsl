#include "DebugTexture.hlsli"

float4 PS(VertexPosHTex pIn) : SV_Target
{
    float4 color = g_Texture.Sample(g_Sam, pIn.Tex);
    return color;
}
