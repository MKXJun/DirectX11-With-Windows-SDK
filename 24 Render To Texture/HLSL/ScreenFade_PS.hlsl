#include "ScreenFade.hlsli"

// ÏñËØ×ÅÉ«Æ÷
float4 PS(VertexPosHTex pIn) : SV_Target
{
    return gTex.Sample(gSam, pIn.Tex) * float4(gFadeAmount, gFadeAmount, gFadeAmount, 1.0f);
}
