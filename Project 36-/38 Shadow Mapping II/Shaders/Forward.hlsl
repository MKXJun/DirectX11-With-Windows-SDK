
#ifndef FORWARD_HLSL
#define FORWARD_HLSL

#include "Rendering.hlsl"

//--------------------------------------------------------------------------------------
// 计算点光源着色 
float4 ForwardPS(VertexOut input) : SV_Target
{
    SurfaceData surface = ComputeSurfaceDataFromGeometry(input);
    
    float3 litColor = BlinnPhong(surface);
    
    return float4(litColor, 1.0f);
}

#endif // FORWARD_HLSL
