
#ifndef CASCADED_SHADOW_HLSL
#define CASCADED_SHADOW_HLSL

#include "ConstantBuffers.hlsl"

// 使用偏导，将shadow map中的texels映射到正在渲染的图元的观察空间平面上
// 该深度将会用于比较并减少阴影走样
// 这项技术是开销昂贵的，且假定对象是平面较多的时候才有效
#ifndef USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG
#define USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG 0
#endif

// 允许在不同级联之间对阴影值混合。当shadow maps比较小
// 且artifacts在两个级联之间可见的时候最为有效
#ifndef BLEND_BETWEEN_CASCADE_LAYERS_FLAG
#define BLEND_BETWEEN_CASCADE_LAYERS_FLAG 0
#endif

// 有两种方法为当前像素片元选择合适的级联：
// Interval-based Selection 将视锥体的深度分区与像素片元的深度进行比较
// Map-based Selection 找到纹理坐标在shadow map范围中的最小级联
#ifndef SELECT_CASCADE_BY_INTERVAL_FLAG
#define SELECT_CASCADE_BY_INTERVAL_FLAG 0
#endif

// 级联数目
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 3
#endif

// 大多数情况下，使用3-4个级联，并开启BLEND_BETWEEN_CASCADE_LAYERS_FLAG，
// 可以适用于低端PC。高端PC可以处理更大的阴影，以及更大的混合地带
// 在使用更大的PCF核时，可以给高端PC使用基于偏导的深度偏移

Texture2DArray g_ShadowMap : register(t10);
SamplerComparisonState g_SamShadow : register(s10);

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

//--------------------------------------------------------------------------------------
// 为阴影空间的texels计算对应光照空间
//--------------------------------------------------------------------------------------
void CalculateRightAndUpTexelDepthDeltas(float3 shadowTexDDX, float3 shadowTexDDY,
                                         out float upTextDepthWeight,
                                         out float rightTextDepthWeight)
{
    // 这里使用X和Y中的偏导数来计算变换矩阵。我们需要逆矩阵将我们从阴影空间变换到屏幕空间，
    // 因为这些导数能让我们从屏幕空间变换到阴影空间。新的矩阵允许我们从阴影图的texels映射
    // 到屏幕空间。这将允许我们寻找对应深度像素的屏幕空间深度。
    // 这不是一个完美的解决方案，因为它假定场景中的几何体是一个平面。
    // [TODO]一种更准确的寻找实际深度的方法为：采用延迟渲染并采样shadow map。
    
    // 在大多数情况下，使用偏移或方差阴影贴图是一种更好的、能够减少伪影的方法
    
    float2x2 matScreenToShadow = float2x2(shadowTexDDX.xy, shadowTexDDY.xy);
    float det = determinant(matScreenToShadow);
    float invDet = 1.0f / det;
    float2x2 matShadowToScreen = float2x2(
        matScreenToShadow._22 * invDet, matScreenToShadow._12 * -invDet,
        matScreenToShadow._21 * -invDet, matScreenToShadow._11 * invDet);
    
    float2 rightShadowTexelLocation = float2(g_TexelSize, 0.0f);
    float2 upShadowTexelLocation = float2(0.0f, g_TexelSize);
    
    // 通过阴影空间到屏幕空间的矩阵变换右边的texel
    float2 rightTexelDepthRatio = mul(rightShadowTexelLocation, matShadowToScreen);
    float2 upTexelDepthRatio = mul(upShadowTexelLocation, matShadowToScreen);
    
    // 我们现在可以计算在shadow map向右和向上移动时，深度的变化值
    // 我们使用x方向和y方向变换的比值乘上屏幕空间X和Y深度的导数来计算变化值
    upTextDepthWeight =
        upTexelDepthRatio.x * shadowTexDDX.z 
        + upTexelDepthRatio.y * shadowTexDDY.z;
    rightTextDepthWeight =
        rightTexelDepthRatio.x * shadowTexDDX.z 
        + rightTexelDepthRatio.y * shadowTexDDY.z;
}

//--------------------------------------------------------------------------------------
// 使用PCF采样深度图并返回着色百分比
//--------------------------------------------------------------------------------------
float CalculatePCFPercentLit(int currentCascadeIndex,
                             float4 shadowTexCoord, 
                             float rightTexelDepthDelta, 
                             float upTexelDepthDelta,
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
            depthCmp -= g_ShadowBias;
            if (USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG)
            {
                depthCmp += rightTexelDepthDelta * (float)x + upTexelDepthDelta * (float)y;
            }
            // 将变换后的像素深度同阴影图中的深度进行比较
            percentLit += g_ShadowMap.SampleCmpLevelZero(g_SamShadow,
                float3(
                    shadowTexCoord.x + (float)x * g_TexelSize,
                    shadowTexCoord.y + (float)y * g_TexelSize,
                    (float)currentCascadeIndex
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
    float blendInterval = g_CascadeFrustumsEyeSpaceDepthsFloat4[currentCascadeIndex].x;
    
    // 对原项目中这部分代码进行了修正
    if (currentCascadeIndex > 0)
    {
        int blendIntervalbelowIndex = currentCascadeIndex - 1;
        pixelDepth -= g_CascadeFrustumsEyeSpaceDepthsFloat4[blendIntervalbelowIndex].x;
        blendInterval -= g_CascadeFrustumsEyeSpaceDepthsFloat4[blendIntervalbelowIndex].x;
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

    float blurSize = g_PCFBlurForLoopEnd - g_PCFBlurForLoopStart;
    blurSize *= blurSize;
         
    int cascadeFound = 0;
    nextCascadeIndex = 1;
    
    //
    // 确定级联，变换阴影纹理坐标
    //
    
    // 当视锥体是均匀划分 且 使用了Interval-Based Selection技术时
    // 可以不需要循环来查找
    // 在这种情况下currentPixelDepth可以用于正确的视锥体数组中查找
    // Interval-Based Selection
    if (SELECT_CASCADE_BY_INTERVAL_FLAG)
    {
        currentCascadeIndex = 0;
        //                               Depth
        // /-+-------/----------------/----+-------/----------/
        // 0 N     F[0]     ...      F[i]        F[i+1] ...   F
        // Depth > F[i] to F[0] => index = i+1
        if (CASCADE_COUNT_FLAG > 1)
        {
            float4 currentPixelDepthVec = currentPixelDepth;
            float4 cmpVec1 = (currentPixelDepthVec > g_CascadeFrustumsEyeSpaceDepthsFloat[0]);
            float4 cmpVec2 = (currentPixelDepthVec > g_CascadeFrustumsEyeSpaceDepthsFloat[1]);
            float index = dot(float4(CASCADE_COUNT_FLAG > 0,
                                     CASCADE_COUNT_FLAG > 1,
                                     CASCADE_COUNT_FLAG > 2,
                                     CASCADE_COUNT_FLAG > 3),
                              cmpVec1) +
                          dot(float4(CASCADE_COUNT_FLAG > 4,
                                     CASCADE_COUNT_FLAG > 5,
                                     CASCADE_COUNT_FLAG > 6,
                                     CASCADE_COUNT_FLAG > 7),
                              cmpVec2);
            index = min(index, CASCADE_COUNT_FLAG - 1);
            currentCascadeIndex = (int) index;
        }
        
        shadowMapTexCoord = shadowMapTexCoordViewSpace * g_CascadeScale[currentCascadeIndex] + g_CascadeOffset[currentCascadeIndex];
    }

    // Map-Based Selection
    if ( !SELECT_CASCADE_BY_INTERVAL_FLAG )
    {
        currentCascadeIndex = 0;
        if (CASCADE_COUNT_FLAG == 1)
        {
            shadowMapTexCoord = shadowMapTexCoordViewSpace * g_CascadeScale[0] + g_CascadeOffset[0];
        }
        if (CASCADE_COUNT_FLAG > 1)
        {
            // 寻找最近的级联，使得纹理坐标位于纹理边界内
            // minBorder < tx, ty < maxBorder
            for (int cascadeIndex = 0; cascadeIndex < CASCADE_COUNT_FLAG && cascadeFound == 0; ++cascadeIndex)
            {
                shadowMapTexCoord = shadowMapTexCoordViewSpace * g_CascadeScale[cascadeIndex] + g_CascadeOffset[cascadeIndex];
                if (min(shadowMapTexCoord.x, shadowMapTexCoord.y) > g_MinBorderPadding
                    && max(shadowMapTexCoord.x, shadowMapTexCoord.y) < g_MaxBorderPadding)
                {
                    currentCascadeIndex = cascadeIndex;
                    cascadeFound = 1;
                }
            }
        }
    }
    
    //
    // 计算当前级联的PCF
    // 
    float3 shadowMapTexCoordDDX;
    float3 shadowMapTexCoordDDY;
    // 这些偏导用于计算投影纹理空间相邻texel对应到光照空间不同方向引起的深度变化
    if (USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG)
    {
        // 计算光照空间的偏导映射到投影纹理空间的变化率
        shadowMapTexCoordDDX = ddx(shadowMapTexCoordViewSpace);
        shadowMapTexCoordDDY = ddy(shadowMapTexCoordViewSpace);
        
        shadowMapTexCoordDDX *= g_CascadeScale[currentCascadeIndex];
        shadowMapTexCoordDDY *= g_CascadeScale[currentCascadeIndex];
        
        CalculateRightAndUpTexelDepthDeltas(shadowMapTexCoordDDX, shadowMapTexCoordDDY,
                                            upTextDepthWeight, rightTextDepthWeight);
    }
    
    visualizeCascadeColor = s_CascadeColorsMultiplier[currentCascadeIndex];
    
    percentLit = CalculatePCFPercentLit(currentCascadeIndex, shadowMapTexCoord,
                                        rightTextDepthWeight, upTextDepthWeight, blurSize);
    
    
    //
    // 在两个级联之间进行混合
    //
    if (BLEND_BETWEEN_CASCADE_LAYERS_FLAG)
    {
        // 为下一个级联重复进行投影纹理坐标的计算
        // 下一级联的索引用于在两个级联之间模糊
        nextCascadeIndex = min(CASCADE_COUNT_FLAG - 1, currentCascadeIndex + 1);
    }
    
    blendBetweenCascadesAmount = 1.0f;
    float currentPixelsBlendBandLocation = 1.0f;
    if (SELECT_CASCADE_BY_INTERVAL_FLAG)
    {
        if (BLEND_BETWEEN_CASCADE_LAYERS_FLAG && CASCADE_COUNT_FLAG > 1)
        {
            CalculateBlendAmountForInterval(currentCascadeIndex, currentPixelDepth,
                currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
        }
    }
    else
    {
        if (BLEND_BETWEEN_CASCADE_LAYERS_FLAG)
        {
            CalculateBlendAmountForMap(shadowMapTexCoord,
                currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
        }
    }
    
    if (BLEND_BETWEEN_CASCADE_LAYERS_FLAG && CASCADE_COUNT_FLAG > 1)
    {
        if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
        {
            // 计算下一级联的投影纹理坐标
            shadowMapTexCoord_blend = shadowMapTexCoordViewSpace * g_CascadeScale[nextCascadeIndex] + g_CascadeOffset[nextCascadeIndex];
            
            // 在级联之间混合时，为下一级联也进行计算
            if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
            {
                // 当前像素在混合地带内
                if (USE_DERIVATIVES_FOR_DEPTH_OFFSET_FLAG)
                {
                    CalculateRightAndUpTexelDepthDeltas(shadowMapTexCoordDDX, shadowMapTexCoordDDY,
                                                        upTextDepthWeight_blend, rightTextDepthWeight_blend);
                }
                percentLit_blend = CalculatePCFPercentLit(nextCascadeIndex, shadowMapTexCoord_blend,
                                                          rightTextDepthWeight_blend, upTextDepthWeight_blend, blurSize);
                // 对两个级联的PCF混合
                percentLit = lerp(percentLit_blend, percentLit, blendBetweenCascadesAmount);
            }
        }
    }

    return percentLit;
}


#endif // CASCADED_SHADOW_HLSL
