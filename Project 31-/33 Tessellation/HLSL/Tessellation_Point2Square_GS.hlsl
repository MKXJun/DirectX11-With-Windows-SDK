#include "Tessellation.hlsli"

[maxvertexcount(4)]
void GS(point VertexOut input[1], inout TriangleStream<GeometryOut> output)
{
    float dx = 10.0f * g_InvScreenHeight;
    
    GeometryOut gOut;
    gOut.PosH = float4(input[0].PosL, 1.0f) + float4(-dx, dx, 0.0f, 0.0f);
    gOut.PosH = mul(gOut.PosH, g_WorldViewProj);
    output.Append(gOut);
    
    gOut.PosH = float4(input[0].PosL, 1.0f) + float4(dx, dx, 0.0f, 0.0f);
    gOut.PosH = mul(gOut.PosH, g_WorldViewProj);
    output.Append(gOut);
    
    gOut.PosH = float4(input[0].PosL, 1.0f) + float4(-dx, -dx, 0.0f, 0.0f);
    gOut.PosH = mul(gOut.PosH, g_WorldViewProj);
    output.Append(gOut);
    
    gOut.PosH = float4(input[0].PosL, 1.0f) + float4(dx, -dx, 0.0f, 0.0f);
    gOut.PosH = mul(gOut.PosH, g_WorldViewProj);
    output.Append(gOut);
}
