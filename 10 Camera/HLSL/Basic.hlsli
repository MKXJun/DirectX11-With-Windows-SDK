#include "LightHelper.hlsli"

Texture2D g_Tex : register(t0);
SamplerState g_SamLinear : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
	matrix g_World;
	matrix g_WorldInvTranspose;
}

cbuffer CBChangesEveryFrame : register(b1)
{
	matrix g_View;
	float3 g_EyePosW;
}

cbuffer CBChangesOnResize : register(b2)
{
	matrix g_Proj;
}

cbuffer CBChangesRarely : register(b3)
{
	DirectionalLight g_DirLight[10];
	PointLight g_PointLight[10];
	SpotLight g_SpotLight[10];
	Material g_Material;
	int g_NumDirLight;
	int g_NumPointLight;
	int g_NumSpotLight;
}



struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosTex
{
    float3 PosL : POSITION;
    float2 Tex : TEXCOORD;
};

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float2 Tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};











