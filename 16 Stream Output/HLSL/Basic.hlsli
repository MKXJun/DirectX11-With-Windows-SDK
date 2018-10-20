#include "LightHelper.hlsli"

cbuffer CBChangesEveryFrame : register(b0)
{
    matrix gWorld;
    matrix gWorldInvTranspose;
}

cbuffer CBChangesOnResize : register(b1)
{
    matrix gProj;
}

cbuffer CBChangesRarely : register(b2)
{
    DirectionalLight gDirLight[5];
    PointLight gPointLight[5];
    SpotLight gSpotLight[5];
    Material gMaterial;
    matrix gView;
    float3 gSphereCenter;
    float gSphereRadius;
    float3 gEyePosW;
    float gPad;
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

struct VertexPosHLColor
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
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

