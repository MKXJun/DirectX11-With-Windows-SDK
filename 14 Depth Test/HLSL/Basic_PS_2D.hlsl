#include "Basic.hlsli"

// 像素着色器(2D)
float4 PS_2D(VertexPosHTex pIn) : SV_Target
{
    float4 color = gTex.Sample(gSam, pIn.Tex);
    clip(color.a - 0.1f);
    return color;
}