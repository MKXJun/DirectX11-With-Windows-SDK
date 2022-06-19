
#ifndef FORWARD_HLSL
#define FORWARD_HLSL

#include "Rendering.hlsl"

//--------------------------------------------------------------------------------------
// 计算点光源着色 
float4 ForwardPS(VertexPosHVNormalVTex input) : SV_Target
{
    uint totalLights, dummy;
    g_Light.GetDimensions(totalLights, dummy);

    float3 litColor = float3(0.0f, 0.0f, 0.0f);

    [branch]
    if (g_VisualizeLightCount)
    {
        litColor = (float(totalLights) * rcp(255.0f)).xxx;
    }
    else
    {
        SurfaceData surface = ComputeSurfaceDataFromGeometry(input);
        for (uint lightIndex = 0; lightIndex < totalLights; ++lightIndex)
        {
            PointLight light = g_Light[lightIndex];
            AccumulateColor(surface, light, litColor);
        }
    }

    return float4(litColor, 1.0f);
}

// 只进行alpha测试，不上色。用于pre-z pass
void ForwardAlphaTestOnlyPS(VertexPosHVNormalVTex input)
{
    // Alpha test: dead code and CSE will take care of the duplication here
    SurfaceData surface = ComputeSurfaceDataFromGeometry(input);
    clip(surface.albedo.a - 0.3f);
}

float4 ForwardAlphaTestPS(VertexPosHVNormalVTex input) : SV_Target
{
    // Always use face normal for alpha tested stuff since it's double-sided
    input.normalV = ComputeFaceNormal(input.posV);

    ForwardAlphaTestOnlyPS(input);

    // Otherwise run the normal shader
    return ForwardPS(input);
}




#endif // FORWARD_HLSL
