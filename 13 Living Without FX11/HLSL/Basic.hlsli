#include "LightHelper.hlsli"

Texture2D tex : register(t0);
SamplerState sam : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
	matrix gWorld;
	matrix gWorldInvTranspose;
	Material gMaterial;
}

cbuffer CBDrawingStates : register(b1)
{
    int gIsReflection;
    int gIsShadow;
}

cbuffer CBChangesEveryFrame : register(b2)
{
	matrix gView;
	float3 gEyePosW;
}

cbuffer CBChangesOnResize : register(b3)
{
	matrix gProj;
}

cbuffer CBChangesRarely : register(b4)
{
    matrix gReflection;
    matrix gShadow;
    matrix gRefShadow;
	DirectionalLight gDirLight[5];
	PointLight gPointLight[5];
	SpotLight gSpotLight[5];
}



struct Vertex3DIn
{
	float3 PosL : POSITION;
    float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
};

struct Vertex3DOut
{
	float4 PosH : SV_POSITION;
    float3 PosW : POSITION;     // 在世界中的位置
    float3 NormalW : NORMAL;    // 法向量在世界中的方向
	float2 Tex : TEXCOORD;
};

struct Vertex2DIn
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD;
};

struct Vertex2DOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};






