#include "Basic.hlsli"

[maxvertexcount(12)]
void GS(triangle VertexPosNormalColor input[3], inout TriangleStream<VertexPosNormalColor> output)
{
    //
    // 将一个三角形分裂成四个三角形，但同时顶点v3, v4, v5也需要在球面上
    //       v1
    //       /\
    //      /  \
    //   v3/____\v4
    //    /\xxxx/\
    //   /  \xx/  \
    //  /____\/____\
    // v0    v5    v2
	
    VertexPosNormalColor vertexes[6];

    matrix viewProj = mul(gView, gProj);

    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        vertexes[i] = input[i];
        vertexes[i + 3].Color = lerp(input[i].Color, input[(i + 1) % 3].Color, 0.5f);
        vertexes[i + 3].NormalL = normalize(input[i].NormalL + input[(i + 1) % 3].NormalL);
        vertexes[i + 3].PosL = gSphereCenter + gSphereRadius * vertexes[i + 3].NormalL;
    }
        
    output.Append(vertexes[0]);
    output.Append(vertexes[3]);
    output.Append(vertexes[5]);
    output.RestartStrip();

    output.Append(vertexes[3]);
    output.Append(vertexes[4]);
    output.Append(vertexes[5]);
    output.RestartStrip();

    output.Append(vertexes[5]);
    output.Append(vertexes[4]);
    output.Append(vertexes[2]);
    output.RestartStrip();

    output.Append(vertexes[3]);
    output.Append(vertexes[1]);
    output.Append(vertexes[4]);
}