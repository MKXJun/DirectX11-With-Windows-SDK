
#ifndef CASCADED_SHADOW_HLSL
#define CASCADED_SHADOW_HLSL

#include "ConstantBuffers.hlsl"

#ifndef MAP_SELECTION
#define MAP_SELECTION 1
#endif

Texture2DArray g_TextureShadow : register(t10);
SamplerComparisonState g_SamplerShadowCmp : register(s10);

static const float4 s_CascadeColorsMultiplier[8] =
{
    float4(1.5f, 0.0f, 0.0f, 1.0f),
    float4(0.0f, 1.5f, 0.0f, 1.0f),
    float4(0.0f, 0.0f, 5.5f, 1.0f),
    float4(1.5f, 0.0f, 5.5f, 1.0f),
    float4(1.5f, 1.5f, 0.0f, 1.0f),
    float4(1.0f, 1.0f, 1.0f, 1.0f),
    float4(0.0f, 1.0f, 5.5f, 1.0f),
    float4(0.5f, 3.5f, 0.75f, 1.0f)
};

float Linstep(float a, float b, float v)
{
    return saturate((v - a) / (b - a));
}

//--------------------------------------------------------------------------------------
// 使用PCF采样深度图并返回着色百分比
//--------------------------------------------------------------------------------------
float CalculatePCFPercentLit(int currentCascadeIndex,
                             float4 shadowTexCoord,
                             float blurSize)
{
    float percentLit = 0.0f;
    // 该循环可以展开，并且如果PCF大小是固定的话，可以使用纹理即时偏移从而改善性能
    for (int x = g_PCFBlurForLoopStart; x < g_PCFBlurForLoopEnd; ++x)
    {
        for (int y = g_PCFBlurForLoopStart; y < g_PCFBlurForLoopEnd; ++y)
        {
            float depthCmp = shadowTexCoord.z;
            // 一个非常简单的解决PCF深度偏移问题的方案是使用一个偏移值
            // 不幸的是，过大的偏移会导致Peter-panning（阴影跑出物体）
            // 过小的偏移又会导致阴影失真
            depthCmp -= g_PCFDepthBias;

            // 将变换后的像素深度同阴影图中的深度进行比较
            percentLit += g_TextureShadow.SampleCmpLevelZero(g_SamplerShadowCmp,
                float3(
                    shadowTexCoord.x + (float) x * g_TexelSize,
                    shadowTexCoord.y + (float) y * g_TexelSize,
                    (float) currentCascadeIndex
                ),
                depthCmp);
        }
    }
    percentLit /= blurSize;
    return percentLit;
}

//--------------------------------------------------------------------------------------
// 计算两个级联之间的混合量 及 混合将会发生的区域
//--------------------------------------------------------------------------------------
void CalculateBlendAmountForInterval(int currentCascadeIndex,
                                     inout float pixelDepth,
                                     inout float currentPixelsBlendBandLocation,
                                     out float blendBetweenCascadesAmount)
{
    
    //                  pixelDepth
    //           |<-      ->|
    // /-+-------/----------+------/--------
    // 0 N     F[0]               F[i]
    //           |<-blendInterval->|
    // blendBandLocation = 1 - depth/F[0] or
    // blendBandLocation = 1 - (depth-F[0]) / (F[i]-F[0])
    // blendBandLocation位于[0, g_CascadeBlendArea]时，进行[0, 1]的过渡
    
    // 我们需要计算当前shadow map的边缘地带，在那里将会淡化到下一个级联
    // 然后我们就可以提前脱离开销昂贵的PCF for循环
    float blendInterval = g_CascadeFrustumsEyeSpaceDepthsData[currentCascadeIndex];
    
    // 对原项目中这部分代码进行了修正
    if (currentCascadeIndex > 0)
    {
        int blendIntervalbelowIndex = currentCascadeIndex - 1;
        pixelDepth -= g_CascadeFrustumsEyeSpaceDepthsData[blendIntervalbelowIndex];
        blendInterval -= g_CascadeFrustumsEyeSpaceDepthsData[blendIntervalbelowIndex];
    }
    
    // 当前像素的混合地带的位置
    currentPixelsBlendBandLocation = 1.0f - pixelDepth / blendInterval;
    // blendBetweenCascadesAmount用于最终的阴影色插值
    blendBetweenCascadesAmount = currentPixelsBlendBandLocation / g_CascadeBlendArea;
}

//--------------------------------------------------------------------------------------
// 计算两个级联之间的混合量 及 混合将会发生的区域
//--------------------------------------------------------------------------------------
void CalculateBlendAmountForMap(float4 shadowMapTexCoord,
                                inout float currentPixelsBlendBandLocation,
                                inout float blendBetweenCascadesAmount)
{
    //   _____________________
    //  |       map[i+1]      |
    //  |                     |
    //  |      0_______0      |
    //  |______| map[i]|______|
    //         |  0.5  |
    //         |_______|
    //         0       0
    // blendBandLocation = min(tx, ty, 1-tx, 1-ty);
    // blendBandLocation位于[0, g_CascadeBlendArea]时，进行[0, 1]的过渡
    float2 distanceToOne = float2(1.0f - shadowMapTexCoord.x, 1.0f - shadowMapTexCoord.y);
    currentPixelsBlendBandLocation = min(shadowMapTexCoord.x, shadowMapTexCoord.y);
    float currentPixelsBlendBandLocation2 = min(distanceToOne.x, distanceToOne.y);
    currentPixelsBlendBandLocation =
        min(currentPixelsBlendBandLocation, currentPixelsBlendBandLocation2);
    
    blendBetweenCascadesAmount = currentPixelsBlendBandLocation / g_CascadeBlendArea;
}

//--------------------------------------------------------------------------------------
// 计算级联显示色或者对应的过渡色
//--------------------------------------------------------------------------------------
float4 GetCascadeColorMultipler(int currentCascadeIndex, 
                                int nextCascadeIndex, 
                                float blendBetweenCascadesAmount)
{
    return lerp(s_CascadeColorsMultiplier[nextCascadeIndex], 
                s_CascadeColorsMultiplier[currentCascadeIndex], 
                blendBetweenCascadesAmount);
}

//--------------------------------------------------------------------------------------
// 计算级联阴影
//--------------------------------------------------------------------------------------
float CalculateCascadedShadow(float4 shadowMapTexCoordViewSpace, 
                              float currentPixelDepth,
                              out int currentCascadeIndex,
                              out int nextCascadeIndex,
                              out float blendBetweenCascadesAmount)
{
    float4 shadowMapTexCoord = 0.0f;
    float4 shadowMapTexCoord_blend = 0.0f;
    
    float4 visualizeCascadeColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    float percentLit = 0.0f;
    float percentLit_blend = 0.0f;

    float upTextDepthWeight = 0;
    float rightTextDepthWeight = 0;
    float upTextDepthWeight_blend = 0;
    float rightTextDepthWeight_blend = 0;
         
    int cascadeFound = 0;
    nextCascadeIndex = 1;
    
    float blurSize = g_PCFBlurForLoopEnd - g_PCFBlurForLoopStart;
    blurSize *= blurSize;
    
    //
    // 确定级联，变换阴影纹理坐标
    //
    
    // 当视锥体是均匀划分 且 使用了Interval-Based Selection技术时
    // 可以不需要循环来查找
    // 在这种情况下currentPixelDepth可以用于正确的视锥体数组中查找
    // Interval-Based Selection
#if MAP_SELECTION == 0
    currentCascadeIndex = 0;
    //                               Depth
    // /-+-------/----------------/----+-------/----------/
    // 0 N     F[0]     ...      F[i]        F[i+1] ...   F
    // Depth > F[i] to F[0] => index = i+1
    float4 currentPixelDepthVec = currentPixelDepth;
    float4 cmpVec1 = (currentPixelDepthVec > g_CascadeFrustumsEyeSpaceDepthsData);
    float index = dot(float4(1.0f, 1.0f, 1.0f, 1.0f), cmpVec1);
    index = min(index, 3);
    currentCascadeIndex = (int) index;
        
    shadowMapTexCoord = shadowMapTexCoordViewSpace * g_CascadeScale[currentCascadeIndex] + g_CascadeOffset[currentCascadeIndex];
#else
    // Map-Based Selection
    currentCascadeIndex = 0;
        // 寻找最近的级联，使得纹理坐标位于纹理边界内
        // minBorder < tx, ty < maxBorder
    for (int cascadeIndex = 0; cascadeIndex < 4 && cascadeFound == 0; ++cascadeIndex)
    {
        shadowMapTexCoord = shadowMapTexCoordViewSpace * g_CascadeScale[cascadeIndex] + g_CascadeOffset[cascadeIndex];
        if (min(shadowMapTexCoord.x, shadowMapTexCoord.y) > g_MinBorderPadding
                    && max(shadowMapTexCoord.x, shadowMapTexCoord.y) < g_MaxBorderPadding)
        {
            currentCascadeIndex = cascadeIndex;
            cascadeFound = 1;
        }
    }
#endif
    
    //
    // 计算当前级联的PCF
    // 
    
    visualizeCascadeColor = s_CascadeColorsMultiplier[currentCascadeIndex];
    
    percentLit = CalculatePCFPercentLit(currentCascadeIndex, shadowMapTexCoord, blurSize);

    
    //
    // 在两个级联之间进行混合
    //
    
    // 为下一个级联重复进行投影纹理坐标的计算
    // 下一级联的索引用于在两个级联之间模糊
    nextCascadeIndex = min(3, currentCascadeIndex + 1);
    
    blendBetweenCascadesAmount = 1.0f;
    float currentPixelsBlendBandLocation = 1.0f;
    
#if MAP_SELECTION == 0
    CalculateBlendAmountForInterval(currentCascadeIndex, currentPixelDepth,
            currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
#else
    CalculateBlendAmountForMap(shadowMapTexCoord,
            currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
#endif
    
    if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
    {
        // 计算下一级联的投影纹理坐标
        shadowMapTexCoord_blend = shadowMapTexCoordViewSpace * g_CascadeScale[nextCascadeIndex] + g_CascadeOffset[nextCascadeIndex];
            
        // 在级联之间混合时，为下一级联也进行计算
        if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
        {
            percentLit_blend = CalculatePCFPercentLit(nextCascadeIndex, shadowMapTexCoord_blend, blurSize);
                
            // 对两个级联的PCF混合
            percentLit = lerp(percentLit_blend, percentLit, blendBetweenCascadesAmount);
        }
    }

    return percentLit;
}


#endif // CASCADED_SHADOW_HLSL
