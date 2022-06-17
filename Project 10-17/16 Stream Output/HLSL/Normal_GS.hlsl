#include "Basic.hlsli"

[maxvertexcount(2)]
void GS(point VertexPosHWNormalColor input[1], inout LineStream<VertexPosHWNormalColor> output)
{
    matrix viewProj = mul(g_View, g_Proj);
    

    VertexPosHWNormalColor v;

    // 防止资源争夺
    v.posW = input[0].posW + input[0].normalW * 0.01f;
    v.normalW = input[0].normalW;
    v.posH = mul(float4(v.posW, 1.0f), viewProj);
    v.color = input[0].color;
    output.Append(v);

    v.posW = v.posW + input[0].normalW * 0.5f;
    v.posH = mul(float4(v.posW, 1.0f), viewProj);

    output.Append(v);
}