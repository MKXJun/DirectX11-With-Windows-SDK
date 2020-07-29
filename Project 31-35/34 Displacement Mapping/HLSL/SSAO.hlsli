
Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_NormalDepthMap : register(t2);
Texture2D g_RandomVecMap : register(t3);
Texture2D g_InputImage : register(t4);

SamplerState g_SamLinearWrap : register(s0);
SamplerState g_SamNormalDepth : register(s1);
SamplerState g_SamRandomVec : register(s2);
SamplerState g_SamBlur : register(s3); // MIG_MAG_LINEAR_MIP_POINT CLAMP

cbuffer CBChangesEveryObjectDrawing : register(b0)
{
    //
    // 用于SSAO_NormalDepth
    //
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_WorldView;
    matrix g_WorldViewProj;
    matrix g_WorldInvTransposeView;
}

cbuffer CBChangesEveryFrame : register(b1)
{
    matrix g_View;
    matrix g_ViewProj;
    float3 g_EyePosW;
    float g_HeightScale;
    float g_MaxTessDistance;
    float g_MinTessDistance;
    float g_MinTessFactor;
    float g_MaxTessFactor;
}

cbuffer CBChangesOnResize : register(b2)
{
    matrix g_Proj;
    
    //
    // 用于SSAO
    //
    matrix g_ViewToTexSpace;    // Proj * Texture
    float4 g_FrustumCorners[4]; // 视锥体远平面的4个端点
}

cbuffer CBChangesRarely : register(b3)
{
    // 14个方向均匀分布但长度随机的向量
    float4 g_OffsetVectors[14]; 
    
    // 观察空间下的坐标
    float g_OcclusionRadius = 0.5f;
    float g_OcclusionFadeStart = 0.2f;
    float g_OcclusionFadeEnd = 2.0f;
    float g_SurfaceEpsilon = 0.05f;
    
    //
    // 用于SSAO_Blur
    //
    float4 g_BlurWeights[3] =
    {
        float4(0.05f, 0.05f, 0.1f, 0.1f),
        float4(0.1f, 0.2f, 0.1f, 0.1f),
        float4(0.1f, 0.05f, 0.05f, 0.0f)
    };
    
    int g_BlurRadius = 5;
    int3 g_Pad;
};

//
// 用于SSAO_NormalDepth和SSAO_Blur
//
struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct InstancePosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    matrix World : World;
    matrix WorldInvTranspose : WorldInvTranspose;
};

struct VertexPosHVNormalVTex
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float3 NormalV : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

struct TessVertexOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    float TessFactor : TESS;
};

struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
};

//
// 用于SSAO
//
struct VertexIn
{
    float3 PosL : POSITION;
    float3 ToFarPlaneIndex : NORMAL; // 仅使用x分量来进行对视锥体远平面顶点的索引
    float2 Tex : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 ToFarPlane : TEXCOORD0; // 远平面顶点坐标
    float2 Tex : TEXCOORD1;
};
