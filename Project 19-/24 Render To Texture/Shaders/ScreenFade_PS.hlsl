#include "PostProcess.hlsli"

// 像素着色器
float4 PS(VertexPosHTex pIn, uniform float fadeAmount) : SV_Target
{
    return g_Tex.Sample(g_Sam, pIn.tex) * float4(fadeAmount, fadeAmount, fadeAmount, 1.0f);
}
