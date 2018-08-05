#include "Basic.fx"

// ÏñËØ×ÅÉ«Æ÷(2D)
float4 PS_2D(Vertex2DOut pIn) : SV_Target
{
    float4 color = tex.Sample(sam, pIn.Tex);
    clip(color.a - 0.1f);
    return color;
}