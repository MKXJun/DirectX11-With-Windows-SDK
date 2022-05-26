#ifndef FXAA_HLSL_INC
#define FXAA_HLSL_INC

#ifndef FXAA_QUALITY__PRESET
#define FXAA_QUALITY__PRESET 39
#endif

//   FXAA 质量 - 低质量，中等抖动
#if (FXAA_QUALITY__PRESET == 10)
#define FXAA_QUALITY__PS 3 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.5, 3.0, 12.0 };
#endif

#if (FXAA_QUALITY__PRESET == 11)
#define FXAA_QUALITY__PS 4 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 3.0, 12.0 };
#endif

#if (FXAA_QUALITY__PRESET == 12)
#define FXAA_QUALITY__PS 5 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 4.0, 12.0 };
#endif

#if (FXAA_QUALITY__PRESET == 13)
#define FXAA_QUALITY__PS 6 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 4.0, 12.0 };
#endif

#if (FXAA_QUALITY__PRESET == 14)
#define FXAA_QUALITY__PS 7 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 4.0, 12.0 };
#endif

#if (FXAA_QUALITY__PRESET == 15)
#define FXAA_QUALITY__PS 8 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 12.0 };
#endif

//   FXAA 质量 - 中等，较少抖动
#if (FXAA_QUALITY__PRESET == 20)
#define FXAA_QUALITY__PS 3 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.5, 2.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 21)
#define FXAA_QUALITY__PS 4 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 22)
#define FXAA_QUALITY__PS 5 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 23)
#define FXAA_QUALITY__PS 6 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 24)
#define FXAA_QUALITY__PS 7 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 3.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 25)
#define FXAA_QUALITY__PS 8 
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 26)
#define FXAA_QUALITY__PS 9
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 27)
#define FXAA_QUALITY__PS 10
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 28)
#define FXAA_QUALITY__PS 11
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

#if (FXAA_QUALITY__PRESET == 29)
#define FXAA_QUALITY__PS 12
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

//   FXAA 质量 - 高
#if (FXAA_QUALITY__PRESET == 39)
#define FXAA_QUALITY__PS 12
static const float s_SampleDistances[FXAA_QUALITY__PS] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };
#endif

cbuffer CB : register(b0)
{
    float2 g_TexelSize;
    
    // 影响锐利程度
    // 1.00 - 柔和
    // 0.75 - 默认滤波值
    // 0.50 - 更锐利，移除更少的亚像素走样
    // 0.25 - 几乎关掉
    // 0.00 - 完全关掉
    float g_QualitySubPix;
    
    // 所需局部对比度的阈值控制
    // 0.333 - 非常低（更快）
    // 0.250 - 低质量
    // 0.166 - 默认
    // 0.125 - 高质量
    // 0.063 - 非常高（更慢）
    float g_QualityEdgeThreshold;
    
    // 对暗部区域不进行处理的阈值
    // 0.0833 - 默认
    // 0.0625 - 稍快
    // 0.0312 - 更慢
    float g_QualityEdgeThresholdMin;
    
    //
    // FXAA_CS使用
    //
    
    uint   g_LastQueueIndex;
    uint2  g_StartPixel;
}

SamplerState g_SamplerLinearClamp : register(s0);

//
// FXAA_CS使用
//

RWByteAddressBuffer g_WorkCountRW : register(u0);
RWByteAddressBuffer g_WorkQueueRW : register(u1);
RWBuffer<float4> g_ColorQueueRW : register(u2);

ByteAddressBuffer g_WorkCount : register(t0);
ByteAddressBuffer g_WorkQueue : register(t1);
Buffer<float4> g_ColorQueue : register(t2);


#if SUPPORT_TYPED_UAV_LOADS == 1
RWTexture2D<float4> g_ColorOutput : register(u3); // Pass2使用
Texture2D<float4> g_ColorInput : register(t3); // R8G8B8A8_UNORM
float3 FetchColor(int2 st) { return g_Color[st].rgb; }
#else
RWTexture2D<uint> g_ColorOutput : register(u3);
Texture2D<uint> g_ColorInput : register(t3);

uint PackColor(float4 color)
{
    uint R = uint(color.r * 255);
    uint G = uint(color.g * 255) << 8;
    uint B = uint(color.b * 255) << 16;
    uint A = uint(color.a * 255) << 24;
    uint packedColor = R | G | B | A;
    return packedColor;
}

float4 FetchColor(int2 st) 
{
    uint packedColor = g_ColorInput[st];
    return float4((packedColor & 0xFF),
                  ((packedColor >> 8) & 0xFF),
                  ((packedColor >> 16) & 0xFF),
                  ((packedColor >> 24) & 0xFF)) / 255.0f;
}
#endif

// 如果使用预计算的luma，则作为纹理读取，否则输出给pass2使用
#if USE_LUMA_INPUT_BUFFER == 1
Texture2D<float> g_Luma : register(t4);
#else
RWTexture2D<float> g_Luma : register(u4);
#endif

//
// FXAA使用
//
Texture2D g_TextureInput : register(t5);



//
// 亮度计算
//


float RGBToLuminance(float3 LinearRGB)
{
    return sqrt(dot(LinearRGB, float3(0.299f, 0.587f, 0.114f)));
}

float RGBToLogLuminance(float3 LinearRGB)
{
    float Luma = dot(LinearRGB, float3(0.212671, 0.715160, 0.072169));
    return log2(1 + Luma * 15) / 4;
}

#endif
