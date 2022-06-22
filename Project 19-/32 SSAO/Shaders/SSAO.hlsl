#ifndef SSAO_HLSL
#define SSAO_HLSL

#include "FullScreenTriangle.hlsl"

Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalDepthMap : register(t1);
Texture2D g_RandomVecMap : register(t2);
Texture2D g_InputImage : register(t3);

SamplerState g_SamLinearWrap : register(s0);
SamplerState g_SamLinearClamp : register(s1);
SamplerState g_SamNormalDepth : register(s2);
SamplerState g_SamRandomVec : register(s3);
SamplerState g_SamBlur : register(s4); // MIG_MAG_LINEAR_MIP_POINT CLAMP

cbuffer CBChangesEveryObjectDrawing : register(b0)
{
    //
    // 用于SSAO_NormalDepth
    //
    matrix g_WorldView;
    matrix g_WorldViewProj;
    matrix g_WorldInvTransposeView;
}

cbuffer CBChangesOnResize : register(b2)
{
    //
    // 用于SSAO
    //
    matrix g_ViewToTexSpace;    // Proj * Texture
    float4 g_FarPlanePoints[3]; // 远平面三角形(覆盖四个角点)，三角形见下
    float2 g_TexelSize;         // (1.0f / W, 1.0f / H)
}

cbuffer CBChangesRarely : register(b3)
{
    // 14个方向均匀分布但长度随机的向量
    float4 g_OffsetVectors[14]; 
    
    // 观察空间下的坐标
    float g_OcclusionRadius;
    float g_OcclusionFadeStart;
    float g_OcclusionFadeEnd;
    float g_SurfaceEpsilon;
    
    //
    // 用于SSAO_Blur
    //
    float4 g_BlurWeights[3];
    // 该静态变量不属于常量缓冲区
    static float s_BlurWeights[12] = (float[12]) g_BlurWeights;
    
    int g_BlurRadius;
    float3 g_Pad;
};

//
// Pass1 几何Buffer生成
//

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 tex : TEXCOORD;
};

struct VertexPosHVNormalVTex
{
    float4 posH : SV_POSITION;
    float3 posV : POSITION;
    float3 NormalV : NORMAL;
    float2 tex : TEXCOORD0;
};

// 生成观察空间的法向量和深度值的RTT的顶点着色器
VertexPosHVNormalVTex GeometryVS(VertexPosNormalTex vIn)
{
    VertexPosHVNormalVTex vOut;
    
    // 变换到观察空间
    vOut.posV = mul(float4(vIn.posL, 1.0f), g_WorldView).xyz;
    vOut.NormalV = mul(vIn.normalL, (float3x3) g_WorldInvTransposeView);
    
    // 变换到裁剪空间
    vOut.posH = mul(float4(vIn.posL, 1.0f), g_WorldViewProj);
    
    vOut.tex = vIn.tex;
    
    return vOut;
}

// 生成观察空间的法向量和深度值的RTT的像素着色器
float4 GeometryPS(VertexPosHVNormalVTex pIn, uniform bool alphaClip) : SV_TARGET
{
    // 将法向量给标准化
    pIn.NormalV = normalize(pIn.NormalV);
    
    if (alphaClip)
    {
        float4 g_TexColor = g_DiffuseMap.Sample(g_SamLinearWrap, pIn.tex);
        
        clip(g_TexColor.a - 0.1f);
    }
    
    // 返回观察空间的法向量和深度值
    return float4(pIn.NormalV, pIn.posV.z);
}

//
// Pass2 计算AO
//

// 给定点r和p的深度差，计算出采样点q对点p的遮蔽程度
float OcclusionFunction(float distZ)
{
    //
    // 如果depth(q)在depth(p)之后(超出半球范围)，那点q不能遮蔽点p。此外，如果depth(q)和depth(p)过于接近，
    // 我们也认为点q不能遮蔽点p，因为depth(p)-depth(r)需要超过用户假定的Epsilon值才能认为点q可以遮蔽点p
    //
    // 我们用下面的函数来确定遮蔽程度
    //
    //    /|\ Occlusion
    // 1.0 |      ---------------\
    //     |      |             |  \
    //     |                         \
    //     |      |             |      \
    //     |                             \
    //     |      |             |          \
    //     |                                 \
    // ----|------|-------------|-------------|-------> zv
    //     0     Eps          zStart         zEnd
    float occlusion = 0.0f;
    if (distZ > g_SurfaceEpsilon)
    {
        float fadeLength = g_OcclusionFadeEnd - g_OcclusionFadeStart;
        // 当distZ由g_OcclusionFadeStart逐渐趋向于g_OcclusionFadeEnd，遮蔽值由1线性减小至0
        occlusion = saturate((g_OcclusionFadeEnd - distZ) / fadeLength);
    }
    return occlusion;
}

// 使用一个三角形覆盖NDC空间 
// (-1, 1)________ (3, 1)
//        |   |  /
// (-1,-1)|___|/ (1, -1)   
//        |  /
// (-1,-3)|/    
void SSAO_VS(uint vertexID : SV_VertexID,
             out float4 posH : SV_position,
             out float3 farPlanePoint : POSITION,
             out float2 texcoord : TEXCOORD)
{
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    float2 xy = grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    
    texcoord = grid * float2(1.0f, 1.0f);
    posH = float4(xy, 1.0f, 1.0f);
    farPlanePoint = g_FarPlanePoints[vertexID].xyz;
}

// 绘制SSAO图的像素着色器
float4 SSAO_PS(float4 posH : SV_position,
               float3 farPlanePoint : POSITION,
               float2 texcoord : TEXCOORD,
               uniform int sampleCount) : SV_TARGET
{
    // p -- 我们要计算的环境光遮蔽目标点
    // n -- 点p的法向量
    // q -- 点p处所在半球内的随机一点
    // r -- 有可能遮挡点p的一点
    
    // 获取观察空间的法向量和当前像素的z坐标
    float4 normalDepth = g_NormalDepthMap.SampleLevel(g_SamNormalDepth, texcoord, 0.0f);
    
    float3 n = normalDepth.xyz;
    float pz = normalDepth.w;
    
    //
    // 重建观察空间坐标 (x, y, z)
    // 寻找t使得能够满足 p = t * pIn.ToFarPlane
    // p.z = t * pIn.ToFarPlane.z
    // t = p.z / pIn.ToFarPlane.z
    //
    float3 p = (pz / farPlanePoint.z) * farPlanePoint;
    
    // 获取随机向量并从[0, 1]^3映射到[-1, 1]^3
    float3 randVec = g_RandomVecMap.SampleLevel(g_SamRandomVec, 4.0f * texcoord, 0.0f).xyz;
    randVec = 2.0f * randVec - 1.0f;
    
    float occlusionSum = 0.0f;
    
    // 在以p为中心的半球内，根据法线n对p周围的点进行采样
    for (int i = 0; i < sampleCount; ++i)
    {
        // 偏移向量都是固定且均匀分布的(所以我们采用的偏移向量不会在同一方向上扎堆)。
        // 如果我们将这些偏移向量关联于一个随机向量进行反射，则得到的必定为一组均匀分布
        // 的随机偏移向量
        float3 offset = reflect(g_OffsetVectors[i].xyz, randVec);
        
        // 如果偏移向量位于(p, n)定义的平面之后，将其翻转
        float flip = sign(dot(offset, n));
        
        // 在点p处于遮蔽半径的半球范围内进行采样
        float3 q = p + flip * g_OcclusionRadius * offset;
    
        // 将q进行投影，得到投影纹理坐标
        float4 projQ = mul(float4(q, 1.0f), g_ViewToTexSpace);
        projQ /= projQ.w;
        
        // 找到眼睛观察点q方向所能观察到的最近点r所处的深度值(有可能点r不存在，此时观察到
        // 的是远平面上一点)。为此，我们需要查看此点在深度图中的深度值
        float rz = g_NormalDepthMap.SampleLevel(g_SamNormalDepth, projQ.xy, 0.0f).w;
        
        // 重建点r在观察空间中的坐标 r = (rx, ry, rz)
        // 我们知道点r位于眼睛到点q的射线上，故有r = t * q
        // r.z = t * q.z ==> t = t.z / q.z
        float3 r = (rz / q.z) * q;
        
        // 测试点r是否遮蔽p
        //   - 点积dot(n, normalize(r - p))度量遮蔽点r到平面(p, n)前侧的距离。越接近于
        //     此平面的前侧，我们就给它设定越大的遮蔽权重。同时，这也能防止位于倾斜面
        //     (p, n)上一点r的自阴影所产生出错误的遮蔽值(通过设置g_SurfaceEpsilon)，这
        //     是因为在以观察点的视角来看，它们有着不同的深度值，但事实上，位于倾斜面
        //     (p, n)上的点r却没有遮挡目标点p
        //   - 遮蔽权重的大小取决于遮蔽点与其目标点之间的距离。如果遮蔽点r离目标点p过
        //     远，则认为点r不会遮挡点p
        
        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);
        float occlusion = dp * OcclusionFunction(distZ);
        
        occlusionSum += occlusion;
    }
    
    occlusionSum /= sampleCount;
    
    float access = 1.0f - occlusionSum;
    
    // 增强SSAO图的对比度，是的SSAO图的效果更加明显
    return saturate(pow(access, 4.0f));
}

//
// Pass3 模糊AO
//

// 双边滤波

float4 BilateralPS(float4 posH : SV_position,
                   float2 texcoord : TEXCOORD) : SV_Target
{
    // 总是把中心值加进去计算
    float4 color = s_BlurWeights[g_BlurRadius] * g_InputImage.SampleLevel(g_SamBlur, texcoord, 0.0f);
    float totalWeight = s_BlurWeights[g_BlurRadius];
    
    float4 centerNormalDepth = g_NormalDepthMap.SampleLevel(g_SamBlur, texcoord, 0.0f);
    // 分拆出观察空间的法向量和深度
    float3 centerNormal = centerNormalDepth.xyz;
    float centerDepth = centerNormalDepth.w;
    
    for (int i = -g_BlurRadius; i <= g_BlurRadius; ++i)
    {
        // 前面已经加上中心值了
        if (i == 0)
            continue;
#if defined BLUR_HORZ
        float2 offset = float2(g_TexelSize.x * i, 0.0f);
        float4 neighborNormalDepth = g_NormalDepthMap.SampleLevel(g_SamBlur, texcoord + offset, 0.0f);
#else
        float2 offset = float2(0.0f, g_TexelSize.y * i);
        float4 neighborNormalDepth = g_NormalDepthMap.SampleLevel(g_SamBlur, texcoord + offset, 0.0f);
#endif
      
        // 分拆出法向量和深度
        float3 neighborNormal = neighborNormalDepth.xyz;
        float neighborDepth = neighborNormalDepth.w;
        
        //
        // 如果中心值和相邻值的深度或法向量相差太大，我们就认为当前采样点处于边缘区域，
        // 因此不考虑加入当前相邻值
        // 中心值直接加入
        //
        
        if (dot(neighborNormal, centerNormal) >= 0.8f && abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = s_BlurWeights[i + g_BlurRadius];
            
            // 将相邻像素加入进行模糊
#if defined BLUR_HORZ
            color += weight * g_InputImage.SampleLevel(g_SamBlur, texcoord + offset, 0.0f);
#else
            color += weight * g_InputImage.SampleLevel(g_SamBlur, texcoord + offset, 0.0f);
#endif
            totalWeight += weight;
        }
        
    }

    // 通过让总权值变为1来补偿丢弃的采样像素
    return color / totalWeight;
}


float4 DebugAO_PS(float4 posH : SV_position,
                  float2 texCoord : TEXCOORD) : SV_Target
{
    float depth = g_DiffuseMap.Sample(g_SamLinearWrap, texCoord).r;
    return float4(depth.rrr, 1.0f);
}


#endif
