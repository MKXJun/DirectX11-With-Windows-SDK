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
    float3 g_SphereCenter;
    float g_SphereRadius;
    float3 g_EyePosW;
    float g_Pad;
}


struct VertexPosColor
{
    float3 posL : POSITION;
    float4 color : COLOR;
};

struct VertexPosHColor
{
    float4 posH : SV_POSITION;
    float4 color : COLOR;
};

struct VertexPosHLColor
{
    float4 posH : SV_POSITION;
    float3 posL : POSITION;
    float4 color : COLOR;
};


struct VertexPosNormalColor
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float4 color : COLOR;
};

struct VertexPosHWNormalColor
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float4 color : COLOR;
};

