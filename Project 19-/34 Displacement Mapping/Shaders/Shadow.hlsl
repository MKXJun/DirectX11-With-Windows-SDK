#ifndef SHADOW_HLSL
#define SHADOW_HLSL

#include "FullScreenTriangle.hlsl"

Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalMap : register(t1);
SamplerState g_Sam : register(s0);

cbuffer CB : register(b0)
{
    matrix g_World;
    matrix g_WorldViewProj;
    matrix g_WorldInvTranspose;
}

cbuffer CBChangesEveryFrame : register(b1)
{
    matrix g_ViewProj;
    float3 g_EyePosW;
    float g_HeightScale;
    float g_MaxTessDistance;
    float g_MinTessDistance;
    float g_MinTessFactor;
    float g_MaxTessFactor;
}

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 posH : SV_POSITION;
    float2 tex : TEXCOORD;
};

struct TessVertexOut
{
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 tex : TEXCOORD;
    float tessFactor : TESS;
};

struct PatchTess
{
    float edgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct HullOut
{
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 tex : TEXCOORD;
};

VertexPosHTex ShadowVS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;

    vOut.posH = mul(float4(vIn.posL, 1.0f), g_WorldViewProj);
    vOut.tex = vIn.tex;

    return vOut;
}

TessVertexOut ShadowTessVS(VertexPosNormalTex vIn)
{
    TessVertexOut vOut;

    vOut.posW = mul(float4(vIn.posL, 1.0f), g_World).xyz;
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.tex = vIn.tex;

    float d = distance(vOut.posW, g_EyePosW);
    
    // 标准化曲面细分因子
    // tessFactor = 
    //   0, d >= g_MinTessDistance
    //   (g_MinTessDistance - d) / (g_MinTessDistance - g_MaxTessDistance), g_MinTessDistance <= d <= g_MaxTessDistance
    //   1, d <= g_MaxTessDistance
    float tess = saturate((g_MinTessDistance - d) / (g_MinTessDistance - g_MaxTessDistance));
    
    // [0, 1] --> [g_MinTessFactor, g_MaxTessFactor]
    vOut.tessFactor = g_MinTessFactor + tess * (g_MaxTessFactor - g_MinTessFactor);
    
    return vOut;
}


PatchTess PatchHS(InputPatch<TessVertexOut, 3> patch,
                  uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // 对每条边的曲面细分因子求平均值，并选择其中一条边的作为其内部的
    // 曲面细分因子。基于边的属性来进行曲面细分因子的计算非常重要，这
    // 样那些与多个三角形共享的边将会拥有相同的曲面细分因子，否则会导
    // 致间隙的产生
    pt.edgeTess[0] = 0.5f * (patch[1].tessFactor + patch[2].tessFactor);
    pt.edgeTess[1] = 0.5f * (patch[2].tessFactor + patch[0].tessFactor);
    pt.edgeTess[2] = 0.5f * (patch[0].tessFactor + patch[1].tessFactor);
    pt.InsideTess = pt.edgeTess[0];
    
    return pt;
}

[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
HullOut ShadowTessHS(InputPatch<TessVertexOut, 3> p,
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
    HullOut hOut;
    
    // 直传
    hOut.posW = p[i].posW;
    hOut.normalW = p[i].normalW;
    hOut.tex = p[i].tex;
    
    return hOut;
}



[domain("tri")]
VertexPosHTex ShadowTessDS(PatchTess patchTess,
             float3 bary : SV_DomainLocation,
             const OutputPatch<HullOut, 3> tri)
{
    VertexPosHTex dOut;
    
    // 对面片属性进行插值以生成顶点
    float3 posW    = bary.x * tri[0].posW     + bary.y * tri[1].posW     + bary.z * tri[2].posW;
    float3 normalW = bary.x * tri[0].normalW  + bary.y * tri[1].normalW  + bary.z * tri[2].normalW;
    dOut.tex       = bary.x * tri[0].tex      + bary.y * tri[1].tex      + bary.z * tri[2].tex;
    
    // 对插值后的法向量进行标准化
    normalW = normalize(normalW);
    
    //
    // 位移映射
    //
    
    // 基于摄像机到顶点的距离选取mipmap等级；特别地，对每个MipInterval单位选择下一个mipLevel
    // 然后将mipLevel限制在[0, 6]
    const float MipInterval = 20.0f;
    float mipLevel = clamp((distance(posW, g_EyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
    
    // 对高度图采样（存在法线贴图的alpha通道）
    float h = g_NormalMap.SampleLevel(g_Sam, dOut.tex, mipLevel).a;
    
    // 沿着法向量进行偏移
    posW += (g_HeightScale * (h - 1.0f)) * normalW;
    
    // 投影到齐次裁减空间
    dOut.posH = mul(float4(posW, 1.0f), g_ViewProj);
    
    return dOut;
}

// 这仅仅用于Alpha几何裁剪，以保证阴影的显示正确。
// 对于不需要进行纹理采样操作的几何体可以直接将像素
// 着色器设为nullptr
void ShadowPS(VertexPosHTex pIn, uniform float clipValue)
{
    float4 diffuse = g_DiffuseMap.Sample(g_Sam, pIn.tex);
    
    // 不要将透明像素写入深度贴图
    clip(diffuse.a - clipValue);
}

float4 DebugShadowPS(VertexPosHTex pIn) : SV_Target
{
    float depth = g_DiffuseMap.Sample(g_Sam, pIn.tex).r;
    return float4(depth.rrr, 1.0f);
}

#endif
