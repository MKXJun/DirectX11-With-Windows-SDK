#include "ScreenFade.hlsli"

// ÏñËØ×ÅÉ«Æ÷
float4 PS(VertexPosHTex pIn) : SV_Target
{
    return g_Tex.Sample(g_Sam, pIn.Tex) * float4(g_FadeAmount, g_FadeAmount, g_FadeAmount, 1.0f);
}
