
#ifndef CASCADED_SHADOW_HLSL
#define CASCADED_SHADOW_HLSL

#include "ConstantBuffers.hlsl"

// 0: Cascaded Shadow Map
// 1: Variance Shadow Map
// 2: Exponential Shadow Map
// 3: Exponential Variance Shadow Map 2-Component
// 4: Exponential Variance Shadow Map 4-Component
#ifndef SHADOW_TYPE
#define SHADOW_TYPE 1
#endif

// 有两种方法为当前像素片元选择合适的级联：
// Interval-based Selection 将视锥体的深度分区与像素片元的深度进行比较
// Map-based Selection 找到纹理坐标在shadow map范围中的最小级联
#ifndef SELECT_CASCADE_BY_INTERVAL_FLAG
#define SELECT_CASCADE_BY_INTERVAL_FLAG 0
#endif

// 级联数目
#ifndef CASCADE_COUNT_FLAG
#define CASCADE_COUNT_FLAG 4
#endif

// 大多数情况下，使用3-4个级联，并开启BLEND_BETWEEN_CASCADE_LAYERS_FLAG，
// 可以适用于低端PC。高端PC可以处理更大的阴影，以及更大的混合地带
// 在使用更大的PCF核时，可以给高端PC使用基于偏导的深度偏移

Texture2DArray g_ShadowMap : register(t10);
SamplerComparisonState g_SamShadowCmp : register(s10);
SamplerState g_SamShadow : register(s11);

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

// 令[0, amount]的部分归零并将(amount, 1]重新映射到(0, 1]
float ReduceLightBleeding(float pMax, float amount)
{
    return Linstep(amount, 1.0f, pMax);
}

float2 GetEVSMExponents(float positiveExponent, float negativeExponent, int is16BitShadow)
{
    const float maxExponent = (is16BitShadow ? 5.54f : 42.0f);

    float2 exponents = float2(positiveExponent, negativeExponent);

    // 限制指数范围防止出现溢出
    return min(exponents, maxExponent);
}

// 输入的depth需要在[0, 1]的范围
float2 ApplyEvsmExponents(float depth, float2 exponents)
{
    depth = 2.0f * depth - 1.0f;
    float2 expDepth;
    expDepth.x = exp(exponents.x * depth);
    expDepth.y = -exp(-exponents.y * depth);
    return expDepth;
}

float ChebyshevUpperBound(float2 moments,
                          float receiverDepth,
                          float minVariance,
                          float lightBleedingReduction)
{
    float variance = moments.y - (moments.x * moments.x);
    variance = max(variance, minVariance); // 防止0除
    
    float d = receiverDepth - moments.x;
    float p_max = variance / (variance + d * d);
    
    p_max = ReduceLightBleeding(p_max, lightBleedingReduction);
    
    // 单边切比雪夫
    return (receiverDepth <= moments.x ? 1.0f : p_max);
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
            percentLit += g_ShadowMap.SampleCmpLevelZero(g_SamShadowCmp,
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
// VSM：采样深度图并返回着色百分比
//--------------------------------------------------------------------------------------
float CalculateVarianceShadow(float4 shadowTexCoord, 
                              float4 shadowTexCoordViewSpace, 
                              int currentCascadeIndex)
{
    float percentLit = 0.0f;
    
    float2 moments = 0.0f;
    
    // 为了将求导从动态流控制中拉出来，我们计算观察空间坐标的偏导
    // 从而得到投影纹理空间坐标的偏导
    float3 shadowTexCoordDDX = ddx(shadowTexCoordViewSpace).xyz;
    float3 shadowTexCoordDDY = ddy(shadowTexCoordViewSpace).xyz;
    shadowTexCoordDDX *= g_CascadeScale[currentCascadeIndex].xyz;
    shadowTexCoordDDY *= g_CascadeScale[currentCascadeIndex].xyz;
    
    moments += g_ShadowMap.SampleGrad(g_SamShadow,
                   float3(shadowTexCoord.xy, (float) currentCascadeIndex),
                   shadowTexCoordDDX.xy, shadowTexCoordDDY.xy).xy;
    
    percentLit = ChebyshevUpperBound(moments, shadowTexCoord.z, 0.00001f, g_LightBleedingReduction);
    
    return percentLit;
}

//--------------------------------------------------------------------------------------
// ESM：采样深度图并返回着色百分比
//--------------------------------------------------------------------------------------
float CalculateExponentialShadow(float4 shadowTexCoord,
                                 float4 shadowTexCoordViewSpace,
                                 int currentCascadeIndex)
{
    float percentLit = 0.0f;
    
    float occluder = 0.0f;
    
    float3 shadowTexCoordDDX = ddx(shadowTexCoordViewSpace).xyz;
    float3 shadowTexCoordDDY = ddy(shadowTexCoordViewSpace).xyz;
    shadowTexCoordDDX *= g_CascadeScale[currentCascadeIndex].xyz;
    shadowTexCoordDDY *= g_CascadeScale[currentCascadeIndex].xyz;
    
    occluder += g_ShadowMap.SampleGrad(g_SamShadow,
                    float3(shadowTexCoord.xy, (float) currentCascadeIndex),
                    shadowTexCoordDDX.xy, shadowTexCoordDDY.xy).x;
    
    percentLit = saturate(exp(occluder - g_MagicPower * shadowTexCoord.z));
    
    return percentLit;
}

//--------------------------------------------------------------------------------------
// EVSM：采样深度图并返回着色百分比
//--------------------------------------------------------------------------------------
float CalculateExponentialVarianceShadow(float4 shadowTexCoord,
                                         float4 shadowTexCoordViewSpace,
                                         int currentCascadeIndex)
{
    float percentLit = 0.0f;
    
    float2 exponents = GetEVSMExponents(g_EvsmPosExp, g_EvsmNegExp, g_16BitShadow);
    float2 expDepth = ApplyEvsmExponents(shadowTexCoord.z, exponents);
    float4 moments = 0.0f;
    
    float3 shadowTexCoordDDX = ddx(shadowTexCoordViewSpace).xyz;
    float3 shadowTexCoordDDY = ddy(shadowTexCoordViewSpace).xyz;
    shadowTexCoordDDX *= g_CascadeScale[currentCascadeIndex].xyz;
    shadowTexCoordDDY *= g_CascadeScale[currentCascadeIndex].xyz;
    
    moments += g_ShadowMap.SampleGrad(g_SamShadow,
                    float3(shadowTexCoord.xy, (float) currentCascadeIndex),
                    shadowTexCoordDDX.xy, shadowTexCoordDDY.xy);
    
    percentLit = ChebyshevUpperBound(moments.xy, expDepth.x, 0.00001f, g_LightBleedingReduction);
    if (SHADOW_TYPE == 4)
    {
        float neg = ChebyshevUpperBound(moments.zw, expDepth.y, 0.00001f, g_LightBleedingReduction);
        percentLit = min(percentLit, neg);
    }
    
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
    float blendInterval = g_CascadeFrustumsEyeSpaceDepths[currentCascadeIndex];
    
    // 对原项目中这部分代码进行了修正
    if (currentCascadeIndex > 0)
    {
        int blendIntervalbelowIndex = currentCascadeIndex - 1;
        pixelDepth -= g_CascadeFrustumsEyeSpaceDepths[blendIntervalbelowIndex];
        blendInterval -= g_CascadeFrustumsEyeSpaceDepths[blendIntervalbelowIndex];
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
            float4 cmpVec1 = (currentPixelDepthVec > g_CascadeFrustumsEyeSpaceDepthsData[0]);
            float4 cmpVec2 = (currentPixelDepthVec > g_CascadeFrustumsEyeSpaceDepthsData[1]);
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
    
    visualizeCascadeColor = s_CascadeColorsMultiplier[currentCascadeIndex];
    
    if (SHADOW_TYPE == 0)
        percentLit = CalculatePCFPercentLit(currentCascadeIndex, shadowMapTexCoord, blurSize);
    if (SHADOW_TYPE == 1)
        percentLit = CalculateVarianceShadow(shadowMapTexCoord, shadowMapTexCoordViewSpace, currentCascadeIndex);
    if (SHADOW_TYPE == 2)
        percentLit = CalculateExponentialShadow(shadowMapTexCoord, shadowMapTexCoordViewSpace, currentCascadeIndex);
    if (SHADOW_TYPE >= 3)
        percentLit = CalculateExponentialVarianceShadow(shadowMapTexCoord, shadowMapTexCoordViewSpace, currentCascadeIndex);
    
    //
    // 在两个级联之间进行混合
    //
    
    // 为下一个级联重复进行投影纹理坐标的计算
    // 下一级联的索引用于在两个级联之间模糊
    nextCascadeIndex = min(CASCADE_COUNT_FLAG - 1, currentCascadeIndex + 1);
    
    blendBetweenCascadesAmount = 1.0f;
    float currentPixelsBlendBandLocation = 1.0f;
    if (SELECT_CASCADE_BY_INTERVAL_FLAG)
    {
        if (CASCADE_COUNT_FLAG > 1)
        {
            CalculateBlendAmountForInterval(currentCascadeIndex, currentPixelDepth,
                currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
        }
    }
    else
    {
        if (CASCADE_COUNT_FLAG > 1)
        {
            CalculateBlendAmountForMap(shadowMapTexCoord,
                currentPixelsBlendBandLocation, blendBetweenCascadesAmount);
        }
    }
    
    if (CASCADE_COUNT_FLAG > 1)
    {
        if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
        {
            // 计算下一级联的投影纹理坐标
            shadowMapTexCoord_blend = shadowMapTexCoordViewSpace * g_CascadeScale[nextCascadeIndex] + g_CascadeOffset[nextCascadeIndex];
            
            // 在级联之间混合时，为下一级联也进行计算
            if (currentPixelsBlendBandLocation < g_CascadeBlendArea)
            {
                if (SHADOW_TYPE == 0)
                    percentLit_blend = CalculatePCFPercentLit(nextCascadeIndex, shadowMapTexCoord_blend, blurSize);
                if (SHADOW_TYPE == 1)             
                    percentLit_blend = CalculateVarianceShadow(shadowMapTexCoord_blend, shadowMapTexCoordViewSpace, nextCascadeIndex);
                if (SHADOW_TYPE == 2)
                    percentLit_blend = CalculateExponentialShadow(shadowMapTexCoord_blend, shadowMapTexCoordViewSpace, nextCascadeIndex);
                if (SHADOW_TYPE >= 3)
                    percentLit_blend = CalculateExponentialVarianceShadow(shadowMapTexCoord_blend, shadowMapTexCoordViewSpace, nextCascadeIndex);
                
                // 对两个级联的PCF混合
                percentLit = lerp(percentLit_blend, percentLit, blendBetweenCascadesAmount);
            }
        }
    }

    return percentLit;
}


#endif // CASCADED_SHADOW_HLSL
