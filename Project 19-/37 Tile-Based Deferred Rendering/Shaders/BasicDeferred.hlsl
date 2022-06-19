
#ifndef BASIC_DEFERRED_HLSL
#define BASIC_DEFERRED_HLSL

#include "GBuffer.hlsl"

//--------------------------------------------------------------------------------------

float4 BasicDeferred(float4 posViewport, uint sampleIndex)
{
    uint totalLights, dummy;
    g_Light.GetDimensions(totalLights, dummy);
    
    float3 lit = float3(0.0f, 0.0f, 0.0f);

    [branch]
    if (g_VisualizeLightCount)
    {
        // 用亮度表示该像素会被多少灯光处理
        lit = (float(totalLights) * rcp(255.0f)).xxx;
    }
    else
    {
        SurfaceData surface = ComputeSurfaceDataFromGBufferSample(uint2(posViewport.xy), sampleIndex);

        // 避免对天空盒/背景像素着色
        if (surface.posV.z < g_CameraNearFar.y)
        {
            for (uint lightIndex = 0; lightIndex < totalLights; ++lightIndex)
            {
                PointLight light = g_Light[lightIndex];
                AccumulateColor(surface, light, lit);
            }
        }
    }

    return float4(lit, 1.0f);
}

float4 BasicDeferredPS(float4 posViewport : SV_Position) : SV_Target
{
    return BasicDeferred(posViewport, 0);
}

float4 BasicDeferredPerSamplePS(float4 posViewport : SV_Position,
                            uint sampleIndex : SV_SampleIndex) : SV_Target
{
    float4 result;
    if (g_VisualizePerSampleShading)
    {
        result = float4(1, 0, 0, 1);
    }
    else
    {
        result = BasicDeferred(posViewport, sampleIndex);
    }
    return result;
}




#endif // BASICDEFERRED_HLSL
