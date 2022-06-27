//**************************************************************************
// Modified by X_Jun(MKXJun)
// Source at https://github.com/NVIDIAGameWorks/Falcor/blob/master/Source/RenderPasses/Antialiasing/FXAA/FXAA.slang
// Licensed under the MIT License.
//
/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

#ifndef FXAA_HLSL
#define FXAA_HLSL

#include "FullScreenTriangle.hlsl"

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
}

SamplerState g_SamplerLinearClamp : register(s0);
Texture2D<float4> g_TextureInput : register(t0);



//
// 亮度计算
//


float LinearRGBToLuminance(float3 LinearRGB)
{
    // return dot(LinearRGB, float3(0.299f, 0.587f, 0.114f));
    return dot(LinearRGB, float3(0.212671f, 0.715160, 0.072169));
    // return sqrt(dot(LinearRGB, float3(0.212671f, 0.715160, 0.072169)));
    // return log2(1 + dot(LinearRGB, float3(0.212671, 0.715160, 0.072169)) * 15) / 4;
}


/*--------------------------------------------------------------------------*/
float4 PS(
    float4 posH : SV_Position,
    float2 texCoord : TEXCOORD) : SV_TARGET
{
    float2 posM = texCoord;
    float4 color = g_TextureInput.SampleLevel(g_SamplerLinearClamp, texCoord, 0);
    
    
    //   N
    // W M E
    //   S
    float lumaM = LinearRGBToLuminance(color.rgb);
    float lumaS = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(0, 1)).rgb);
    float lumaE = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(1, 0)).rgb);
    float lumaN = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(0, -1)).rgb);
    float lumaW = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(-1, 0)).rgb);

    //
    // 计算对比度，确定是否应用抗锯齿
    //
    
    // 求出5个像素中的最大/最小相对亮度，得到对比度
    float lumaRangeMax = max(lumaM, max(max(lumaW, lumaE), max(lumaN, lumaS)));
    float lumaRangeMin = min(lumaM, min(min(lumaW, lumaE), min(lumaN, lumaS)));
    float lumaRange = lumaRangeMax - lumaRangeMin;
    // 如果亮度变化低于一个与最大亮度呈正相关的阈值，或者低于一个绝对阈值，说明不是处于边缘区域，不进行任何抗锯齿操作
    bool earlyExit = lumaRange < max(g_QualityEdgeThresholdMin, lumaRangeMax * g_QualityEdgeThreshold);

    // 未达到阈值就提前结束
    if (earlyExit)
        return color;

    //
    // 确定边界是局部水平的还是竖直的
    //
    
    //           
    //  NW N NE          
    //  W  M  E
    //  WS S SE     
    //  edgeHorz = |(NW - W) - (W - WS)| + 2|(N - M) - (M - S)| + |(NE - E) - (E - SE)|
    //  edgeVert = |(NE - N) - (N - NW)| + 2|(E - M) - (M - W)| + |(SE - S) - (S - WS)|
    float lumaNW = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(-1, -1)).rgb);
    float lumaSE = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(1, 1)).rgb);
    float lumaNE = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(1, -1)).rgb);
    float lumaSW = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0, int2(-1, 1)).rgb);

    float lumaNS = lumaN + lumaS;
    float lumaWE = lumaW + lumaE;
    float lumaNESE = lumaNE + lumaSE;
    float lumaNWNE = lumaNW + lumaNE;
    float lumaNWSW = lumaNW + lumaSW;
    float lumaSWSE = lumaSW + lumaSE;

    // 计算水平和垂直对比度
    float edgeHorz = abs(lumaNWSW - 2.0 * lumaW) + abs(lumaNS - 2.0 * lumaM) * 2.0 + abs(lumaNESE - 2.0 * lumaE);
    float edgeVert = abs(lumaSWSE - 2.0 * lumaS) + abs(lumaWE - 2.0 * lumaM) * 2.0 + abs(lumaNWNE - 2.0 * lumaN);

    // 判断是 局部水平边界 还是 局部垂直边界
    bool horzSpan = edgeHorz >= edgeVert;
    
    //
    // 计算梯度、确定边界方向
    //
    float luma1 = horzSpan ? lumaN : lumaW;
    float luma2 = horzSpan ? lumaS : lumaE;
    
    float gradient1 = luma1 - lumaM;
    float gradient2 = luma2 - lumaM;
    // 求出对应方向的梯度，然后进行缩放用于后续比较
    float gradientScaled = max(abs(gradient1), abs(gradient2)) * 0.25f;
    // 哪个方向最陡峭
    bool is1Steepest = abs(gradient1) >= abs(gradient2);
    
    //
    // 当前像素沿梯度方向移动半个texel
    //
    float lengthSign = horzSpan ? g_TexelSize.y : g_TexelSize.x;
    lengthSign = is1Steepest ? -lengthSign : lengthSign;
    
    float2 posB = posM.xy;
    // 半texel偏移
    if (!horzSpan)
        posB.x += lengthSign * 0.5;
    if (horzSpan)
        posB.y += lengthSign * 0.5;
    
    //
    // 计算与posB相邻的两个像素的luma的平均值
    //
    float luma3 = luma1 + lumaM;
    float luma4 = luma2 + lumaM;
    float lumaLocalAvg = luma3;
    if (!is1Steepest)
        lumaLocalAvg = luma4;
    lumaLocalAvg *= 0.5f;
    
    

    //
    // 向两边进行遍历，直到遍历结束或达到非边缘点
    //
    
    // 沿边界向两边偏移
    // 0    0    0
    // <-  posB ->
    // 1    1    1
    float2 offset;
    offset.x = (!horzSpan) ? 0.0 : g_TexelSize.x;
    offset.y = (horzSpan) ? 0.0 : g_TexelSize.y;
    // 负方向偏移
    float2 posN = posB - offset * s_SampleDistances[0];
    // 正方向偏移
    float2 posP = posB + offset * s_SampleDistances[0];
    
    // 对偏移后的点获取luma值，然后计算与中间点luma的差异
    float lumaEndN = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posN, 0).rgb);
    float lumaEndP = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posP, 0).rgb);
    lumaEndN -= lumaLocalAvg;
    lumaEndP -= lumaLocalAvg;
    
    // 如果端点处的luma差异大于局部梯度，说明到达边缘的一侧
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    bool doneNP = doneN && doneP;
    
    // 如果没有到达非边缘点，继续沿着该方向延伸
    if (!doneN)
        posN -= offset * s_SampleDistances[1];
    if (!doneP)
        posP += offset * s_SampleDistances[1];

    // 继续迭代直到两边都到达边缘的一侧，或者达到迭代次数
    if (!doneNP)
    {
        [unroll]
        for (int i = 2; i < FXAA_QUALITY__PS; ++i)
        {
            if (!doneN)
                lumaEndN = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posN.xy, 0).rgb) - lumaLocalAvg;
            if (!doneP)
                lumaEndP = LinearRGBToLuminance(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posP.xy, 0).rgb) - lumaLocalAvg;
            
            doneN = abs(lumaEndN) >= gradientScaled;
            doneP = abs(lumaEndP) >= gradientScaled;
            doneNP = doneN && doneP;
        
            if (!doneN)
                posN -= offset * s_SampleDistances[i];
            if (!doneP)
                posP += offset * s_SampleDistances[i];
            // 两边都到达边缘的一侧就停下
            if (doneNP)
                break;
        }
    }
    
    // 分别计算到两个端点的距离
    float distN = horzSpan ? (posM.x - posN.x) : (posM.y - posN.y);
    float distP = horzSpan ? (posP.x - posM.x) : (posP.y - posM.y);

    // 看当前点到哪一个端点更近，取其距离
    bool directionN = distN < distP;
    float dist = min(distN, distP);
    
    // 两端点间的距离
    float spanLength = (distP + distN);
    
    // 朝着最近端点移动的像素偏移量
    float pixelOffset = -dist / spanLength + 0.5f;
/*--------------------------------------------------------------------------*/
    
    // 当前像素的luma是否小于posB相邻的两个像素的luma的平均值
    bool isLumaMSmaller = lumaM < lumaLocalAvg;
    
    // 判断这是否为一个好的边界
    bool goodSpanN = (lumaEndN < 0.0) != isLumaMSmaller;
    bool goodSpanP = (lumaEndP < 0.0) != isLumaMSmaller;
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    
    // 如果不是的话，不进行偏移
    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
/*--------------------------------------------------------------------------*/
    
    // 求3x3范围像素的亮度变化
    //      [1  2  1]
    // 1/12 [2 -12 2]
    //      [1  2  1]
    float subpixNSWE = lumaNS + lumaWE;
    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float subpixA = (2.0 * subpixNSWE + subpixNWSWNESE) * (1.0 / 12.0) - lumaM;
    // 基于这个亮度变化计算亚像素偏移量
    float subpixB = saturate(abs(subpixA) * (1.0 / lumaRange));
    float subpixC = (-2.0 * subpixB + 3.0) * subpixB * subpixB;
    float subpix = subpixC * subpixC * g_QualitySubPix;
    
    // 选择最大的偏移
    float pixelOffsetSubpix = max(pixelOffsetGood, subpix);
/*--------------------------------------------------------------------------*/
    
    if (!horzSpan)
        posM.x += pixelOffsetSubpix * lengthSign;
    if (horzSpan)
        posM.y += pixelOffsetSubpix * lengthSign;
#ifdef DEBUG_OUTPUT
    return float4(1.0f - 2.0f * pixelOffsetSubpix, 2.0f * pixelOffsetSubpix, 0.0f, 1.0f);
#else
    return float4(g_TextureInput.SampleLevel(g_SamplerLinearClamp, posM, 0).xyz, lumaM);
#endif
}

float4 PassThroughPS(float4 posH : SV_Position,
    float2 texCoord : TEXCOORD) : SV_TARGET
{
    uint2 pos = posH.xy;
    return g_TextureInput[pos];
}

#endif
