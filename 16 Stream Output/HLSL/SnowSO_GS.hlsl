#include "BasicObject.fx"

[maxvertexcount(5)]
void GS(line VertexPosColor input[2], inout LineStream<VertexPosColor> output)
{
    // 要求分形线段按顺时针排布
    // z分量必须相等，因为顶点没有提供法向量无法判断垂直上方向
    //                       v1
    //                       /\
    // ____________ =>  ____/  \____
    // i0         i1   i0  v0  v2  i1
	
    VertexPosColor v0, v1, v2;
    v0.Color = lerp(input[0].Color, input[1].Color, 0.25f);
    v1.Color = lerp(input[0].Color, input[1].Color, 0.5f);
    v2.Color = lerp(input[0].Color, input[1].Color, 0.75f);

    v0.PosL = lerp(input[0].PosL, input[1].PosL, 1.0f / 3.0f);
    v2.PosL = lerp(input[0].PosL, input[1].PosL, 2.0f / 3.0f);

    // xy平面求出它的垂直单位向量
    //     
    //     |
    // ____|_____
    float2 upDir = normalize(input[1].PosL - input[0].PosL).yx;
    float len = length(input[1].PosL.xy - input[0].PosL.xy);
    upDir.x = -upDir.x;

    v1.PosL = lerp(input[0].PosL, input[1].PosL, 0.5f);
    v1.PosL.xy += sqrt(3) / 6.0f * len * upDir;

    output.Append(input[0]);
    output.Append(v0);
    output.Append(v1);
    output.Append(v2);
    output.Append(input[1]);

}