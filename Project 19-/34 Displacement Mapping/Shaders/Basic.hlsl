#ifndef BASIC_HLSL
#define BASIC_HLSL

#include "LightHelper.hlsl"

Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_ShadowMap : register(t2);
Texture2D g_AmbientOcclusionMap : register(t3);
SamplerState g_Sam : register(s0);
SamplerComparisonState g_SamShadow : register(s1);

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_WorldViewProj;
}

cbuffer CBChangesEveryObjectDrawing : register(b1)
{
    Material g_Material;
}

cbuffer CBDrawingStates : register(b2)
{
    float g_DepthBias;
    float g_Pad;
}

cbuffer CBChangesEveryFrame : register(b3)
{
    matrix g_ViewProj;
    matrix g_ShadowTransform; // ShadowView * ShadowProj * T
    float3 g_EyePosW;
    float g_HeightScale;
    float g_MaxTessDistance;
    float g_MinTessDistance;
    float g_MinTessFactor;
    float g_MaxTessFactor;
}

cbuffer CBChangesRarely : register(b4)
{
    DirectionalLight g_DirLight[5];
    PointLight g_PointLight[5];
    SpotLight g_SpotLight[5];
}

struct VertexInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
#if defined USE_NORMAL_MAP
    float4 tangentL : TANGENT;
#endif
    float2 tex : TEXCOORD;
};

struct TessVertexInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float4 tangentL : TANGENT;
    float2 tex : TEXCOORD;
};

struct VertexOutput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION; // 在世界中的位置
    float3 normalW : NORMAL; // 法向量在世界中的方向
#if defined USE_NORMAL_MAP
    float4 tangentW : TANGENT; // 切线在世界中的方向
#endif 
    float2 tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
    float4 SSAOPosH : TEXCOORD2;
};

struct TessVertexOut
{
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float4 tangentW : TANGENT;
    float2 tex : TEXCOORD0;
    float  tessFactor : TESS;
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
    float4 tangentW : TANGENT;
    float2 tex : TEXCOORD;
};

struct DomainOutput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;       // 在世界中的位置
    float3 normalW : NORMAL;      // 法向量在世界中的方向
    float4 tangentW : TANGENT;    // 切线在世界中的方向
    float2 tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
    float4 SSAOPosH : TEXCOORD2;
};


// 顶点着色器
VertexOutput BasicVS(VertexInput vIn)
{
    VertexOutput vOut;
    
    vector posW = mul(float4(vIn.posL, 1.0f), g_World);
    
    vOut.posW = posW.xyz;
    vOut.posH = mul(float4(vIn.posL, 1.0f), g_WorldViewProj); // 保持和SSAO的计算一致
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
#if defined USE_NORMAL_MAP
    vOut.tangentW = float4(mul(vIn.tangentL.xyz, (float3x3) g_World), vIn.tangentL.w);
#endif 
    vOut.tex = vIn.tex;
    vOut.ShadowPosH = mul(posW, g_ShadowTransform);
#if defined USE_SSAO_MAP
    // 从NDC坐标[-1, 1]^2变换到纹理空间坐标[0, 1]^2
    // u = 0.5x + 0.5
    // v = -0.5y + 0.5
    // ((xw, yw, zw, w) + (w, -w, 0, 0)) * (0.5, -0.5, 1, 1) = ((0.5x + 0.5)w, (-0.5y + 0.5)w, zw, w)
    //                                                      = (uw, vw, zw, w)
    //                                                      =>  (u, v, z, 1)
    vOut.SSAOPosH = (vOut.posH + float4(vOut.posH.w, -vOut.posH.w, 0.0f, 0.0f)) * float4(0.5f, -0.5f, 1.0f, 1.0f);
#endif
    return vOut;
}

TessVertexOut TessVS(TessVertexInput vIn)
{
    TessVertexOut vOut;

    vOut.posW = mul(float4(vIn.posL, 1.0f), g_World).xyz;
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.tangentW = float4(mul(vIn.tangentL.xyz, (float3x3) g_World), vIn.tangentL.w);
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

// 外壳着色器
[domain("tri")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("PatchHS")]
HullOut TessHS(InputPatch<TessVertexOut, 3> patch,
    uint i : SV_OutputControlPointID,
    uint patchId : SV_PrimitiveID)
{
    HullOut hOut;
    
    // 直传
    hOut.posW = patch[i].posW;
    hOut.normalW = patch[i].normalW;
    hOut.tangentW = patch[i].tangentW;
    hOut.tex = patch[i].tex;
    
    return hOut;
}

// 域着色器
[domain("tri")]
DomainOutput TessDS(PatchTess patchTess,
             float3 bary : SV_DomainLocation,
             const OutputPatch<HullOut, 3> tri)
{
    DomainOutput dOut;
    
    // 对面片属性进行插值以生成顶点
    dOut.posW     = bary.x * tri[0].posW     + bary.y * tri[1].posW     + bary.z * tri[2].posW;
    dOut.normalW  = bary.x * tri[0].normalW  + bary.y * tri[1].normalW  + bary.z * tri[2].normalW;
    dOut.tangentW = bary.x * tri[0].tangentW + bary.y * tri[1].tangentW + bary.z * tri[2].tangentW;
    dOut.tex      = bary.x * tri[0].tex      + bary.y * tri[1].tex      + bary.z * tri[2].tex;
    
    // 对插值后的法向量和切线进行标准化
    dOut.normalW = normalize(dOut.normalW);
    
    //
    // 位移映射
    //
    
    // 基于摄像机到顶点的距离选取mipmap等级；特别地，对每个MipInterval单位选择下一个mipLevel
    // 然后将mipLevel限制在[0, 6]
    const float MipInterval = 20.0f;
    float mipLevel = clamp((distance(dOut.posW, g_EyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
    
    // 对高度图采样（存在法线贴图的alpha通道）
    float h = g_NormalMap.SampleLevel(g_Sam, dOut.tex, mipLevel).a;
    
    // 沿着法向量进行偏移
    dOut.posW += (g_HeightScale * (h - 1.0f)) * dOut.normalW;
    
    // 生成投影纹理坐标
    dOut.ShadowPosH = mul(float4(dOut.posW, 1.0f), g_ShadowTransform);
    
    // 投影到齐次裁减空间
    dOut.posH = mul(float4(dOut.posW, 1.0f), g_ViewProj);
    
    // 从NDC坐标[-1, 1]^2变换到纹理空间坐标[0, 1]^2
    // u = 0.5x + 0.5
    // v = -0.5y + 0.5
    // ((xw, yw, zw, w) + (w, -w, 0, 0)) * (0.5, -0.5, 1, 1) = ((0.5x + 0.5)w, (-0.5y + 0.5)w, zw, w)
    //                                                      = (uw, vw, zw, w)
    //                                                      =>  (u, v, z, 1)
    dOut.SSAOPosH = (dOut.posH + float4(dOut.posH.w, -dOut.posH.w, 0.0f, 0.0f)) * float4(0.5f, -0.5f, 1.0f, 1.0f);
    
    return dOut;
}

// 像素着色器
float4 BasicPS(VertexOutput pIn) : SV_Target
{
    float4 texColor = g_DiffuseMap.Sample(g_Sam, pIn.tex);
    // 提前进行Alpha裁剪，对不符合要求的像素可以避免后续运算
    clip(texColor.a - 0.1f);
    
    // 标准化法向量
    pIn.normalW = normalize(pIn.normalW);

    // 求出顶点指向眼睛的向量，以及顶点与眼睛的距离
    float3 toEyeW = normalize(g_EyePosW - pIn.posW);
    float distToEye = distance(g_EyePosW, pIn.posW);

#if defined USE_NORMAL_MAP
    // 标准化切线
    pIn.tangentW = normalize(pIn.tangentW);
    // 采样法线贴图
    float3 normalMapSample = g_NormalMap.Sample(g_Sam, pIn.tex).rgb;
    pIn.normalW = NormalSampleToWorldSpace(normalMapSample, pIn.normalW, pIn.tangentW);
#endif
    
    // 完成纹理投影变换并对SSAO图采样
    float ambientAccess = 1.0f;
    pIn.SSAOPosH /= pIn.SSAOPosH.w;
    ambientAccess = g_AmbientOcclusionMap.SampleLevel(g_Sam, pIn.SSAOPosH.xy, 0.0f).r;
    
    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int i;

    float shadow[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    // 仅第一个方向光用于计算阴影
    shadow[0] = CalcShadowFactor(g_SamShadow, g_ShadowMap, pIn.ShadowPosH, g_DepthBias);
    
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeDirectionalLight(g_Material, g_DirLight[i], pIn.normalW, toEyeW, A, D, S);
        ambient += ambientAccess * A;
        diffuse += shadow[i] * D;
        spec += shadow[i] * S;
    }
        
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputePointLight(g_Material, g_PointLight[i], pIn.posW, pIn.normalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeSpotLight(g_Material, g_SpotLight[i], pIn.posW, pIn.normalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
  
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * g_Material.diffuse.a;
    return litColor;
}



#endif
