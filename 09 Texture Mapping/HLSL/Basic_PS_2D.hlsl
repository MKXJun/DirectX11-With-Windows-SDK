#include "Basic.hlsli"

// 像素着色器(2D)
float4 PS_2D(VertexPosHTex pIn) : SV_Target
{
    return gTex.Sample(gSamLinear, pIn.Tex);
}