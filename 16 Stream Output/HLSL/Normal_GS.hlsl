#include "Basic.hlsli"

[maxvertexcount(2)]
void GS(point VertexPosHWNormalColor input[1], inout LineStream<VertexPosHWNormalColor> output)
{
    matrix viewProj = mul(g_View, g_Proj);
    

    VertexPosHWNormalColor v;

    // ·ÀÖ¹×ÊÔ´Õù¶á
    v.PosW = input[0].PosW + input[0].NormalW * 0.01f;
    v.NormalW = input[0].NormalW;
    v.PosH = mul(float4(v.PosW, 1.0f), viewProj);
    v.Color = input[0].Color;
    output.Append(v);

    v.PosW = v.PosW + input[0].NormalW * 0.5f;
    v.PosH = mul(float4(v.PosW, 1.0f), viewProj);

    output.Append(v);
}