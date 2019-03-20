#include "LightHelper.hlsli"

cbuffer CBChangesEveryFrame : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
}

cbuffer CBChangesOnResize : register(b1)
{
    matrix g_Proj;
}

cbuffer CBChangesRarely : register(b2)
{
    DirectionalLight g_DirLight[5];
    PointLight g_PointLight[5];
    SpotLight g_SpotLight[5];
    Material g_Material;
    matrix g_View;
    float3 g_EyePosW;
    float g_CylinderHeight;
}


struct VertexPosColor
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexPosHColor
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};


struct VertexPosNormalColor
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float4 Color : COLOR;
};

struct VertexPosHWNormalColor
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float4 Color : COLOR;
};

