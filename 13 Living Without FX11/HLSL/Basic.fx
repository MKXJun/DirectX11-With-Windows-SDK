#include "LightHelper.hlsli"

Texture2D tex : register(t0);
SamplerState sam : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
	row_major matrix gWorld;
	row_major matrix gWorldInvTranspose;
	row_major matrix gTexTransform;
	Material gMaterial;
}

cbuffer CBDrawingState : register(b1)
{
    int gIsReflection;
    int gIsShadow;
}

cbuffer CBChangesEveryFrame : register(b2)
{
	row_major matrix gView;
	float3 gEyePosW;
}

cbuffer CBChangesOnResize : register(b3)
{
	row_major matrix gProj;
}

cbuffer CBNeverChange : register(b4)
{
    row_major matrix gReflection;
    row_major matrix gShadow;
    row_major matrix gRefShadow;
	DirectionalLight gDirLight[10];
	PointLight gPointLight[10];
	SpotLight gSpotLight[10];
	int gNumDirLight;
	int gNumPointLight;
	int gNumSpotLight;
	float gPad;
}



struct Vertex3DIn
{
	float3 Pos : POSITION;
    float3 Normal : NORMAL;
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






