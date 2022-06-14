#include "LightHelper.hlsli"

Texture2D g_DiffuseMap : register(t0);          // 物体纹理
Texture2D g_DisplacementMap : register(t1);     // 位移贴图
SamplerState g_SamLinearWrap : register(s0);    // 线性过滤+Wrap采样器
SamplerState g_SamPointClamp : register(s1);    // 点过滤+Clamp采样器

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_TexTransform;
}

cbuffer CBChangesEveryObjectDrawing : register(b1)
{
    Material g_Material;
}

cbuffer CBChangesEveryFrame : register(b2)
{
    matrix g_ViewProj;
    float3 g_EyePosW;
    float g_Pad;
}

cbuffer CBDrawingStates : register(b3)
{
    float4 g_FogColor;
    
    int g_FogEnabled;
    float g_FogStart;
    float g_FogRange;
    int g_WavesEnabled;                     // 开启波浪绘制
    
    float g_GridSpatialStep;                // 栅格空间步长
    float3 g_Pad2;
}

cbuffer CBChangesRarely : register(b4)
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

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float2 Tex : TEXCOORD;
};




