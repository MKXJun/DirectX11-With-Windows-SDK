#include "LightHelper.hlsli"

Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_ShadowMap : register(t2);
Texture2D g_SSAOMap : register(t3);
TextureCube g_TexCube : register(t4);
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
    int g_EnableShadow;
    int g_EnableSSAO;
    float2 g_Pad;
}

cbuffer CBChangesEveryFrame : register(b3)
{
    matrix g_View;
    matrix g_ShadowTransform;   // ShadowView * ShadowProj * T
    float3 g_EyePosW;
    float g_Pad2;
}

cbuffer CBChangesOnResize : register(b4)
{
    matrix g_Proj;
}

cbuffer CBChangesRarely : register(b5)
{
    DirectionalLight g_DirLight[5];
    PointLight g_PointLight[5];
    SpotLight g_SpotLight[5];
}

struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosNormalTangentTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float4 TangentL : TANGENT;
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

struct InstancePosNormalTangentTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float4 TangentL : TANGENT;
    float2 Tex : TEXCOORD;
    matrix World : World;
    matrix WorldInvTranspose : WorldInvTranspose;
};

struct VertexOutBasic
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float2 Tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
    float4 SSAOPosH : TEXCOORD2;
};

struct VertexOutNormalMap
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float4 TangentW : TANGENT; // 切线在世界中的方向
    float2 Tex : TEXCOORD0;
    float4 ShadowPosH : TEXCOORD1;
    float4 SSAOPosH : TEXCOORD2;
};

