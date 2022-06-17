#ifndef FULL_SCREEN_TRIANGLE_HLSL
#define FULL_SCREEN_TRIANGLE_HLSL

// 使用一个三角形覆盖NDC空间 
// (-1, 1)________ (3, 1)
//        |   |  /
// (-1,-1)|___|/ (1, -1)   
//        |  /
// (-1,-3)|/      
float4 FullScreenTriangleVS(uint vertexID : SV_VertexID) : SV_Position
{
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    float2 xy = grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    return float4(xy, 1.0f, 1.0f);
}

void FullScreenTriangleTexcoordVS(uint vertexID : SV_VertexID,
                                  out float4 posH : SV_Position,
                                  out float2 texcoord : TEXCOORD)
{
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    float2 xy = grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    texcoord = grid * float2(1.0f, 1.0f);
    posH = float4(xy, 1.0f, 1.0f);
}

#endif // FULL_SCREEN_TRIANGLE_HLSL
