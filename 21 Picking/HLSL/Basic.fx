#include "LightHelper.hlsli"

Texture2D texA : register(t0);
Texture2D texD : register(t1);
SamplerState sam : register(s0);


cbuffer CBChangesEveryObjectDrawing : register(b0)
{
    row_major matrix gWorld;
    row_major matrix gWorldInvTranspose;
}

cbuffer CBChangesEveryInstanceDrawing : register(b1)
{
    Material gMaterial;
}

cbuffer CBDrawingStates : register(b2)
{
    int gTextureUsed;
    float3 gPad;
}

cbuffer CBChangesEveryFrame : register(b3)
{
    row_major matrix gView;
    float3 gEyePosW;
    float gPad2;
}

cbuffer CBChangesOnResize : register(b4)
{
    row_major matrix gProj;
}

cbuffer CBChangesRarely : register(b5)
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
    row_major matrix World : World;
    row_major matrix WorldInvTranspose : WorldInvTranspose;
};



