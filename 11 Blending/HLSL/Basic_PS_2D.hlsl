#include "Basic.hlsli"

// ÏñËØ×ÅÉ«Æ÷(2D)
float4 PS_2D(VertexOut pIn) : SV_Target
{
    float4 color = tex.Sample(samLinear, pIn.Tex);
    clip(color.a - 0.1f);
    return color;
}