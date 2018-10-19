#include "LightHelper.hlsli"

Texture2D texA : register(t0);
Texture2D texD : register(t1);
SamplerState sam : register(s0);


cbuffer CBChangesEveryObjectDrawing : register(b0)
{
    matrix gWorld;
    matrix gWorldInvTranspose;
}

cbuffer CBChangesEveryInstanceDrawing : register(b1)
{
    Material gMaterial;
}

cbuffer CBChangesEveryFrame : register(b2)
{
    matrix gView;
    float3 gEyePosW;
    float gPad;
}

cbuffer CBChangesOnResize : register(b3)
{
    matrix gProj;
}

cbuffer CBChangesRarely : register(b4)
{
    DirectionalLight gDirLight[5];
    PointLight gPointLight[5];
    SpotLight gSpotLight[5];
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

struct InstancePosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
    matrix World : World;
    matrix WorldInvTranspose : WorldInvTranspose;
};



