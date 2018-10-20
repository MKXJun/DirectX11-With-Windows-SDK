#include "LightHelper.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);


cbuffer VSConstantBuffer : register(b0)
{
    matrix gWorld; 
    matrix gView;  
    matrix gProj;  
    matrix gWorldInvTranspose;
}

cbuffer PSConstantBuffer : register(b1)
{
    DirectionalLight gDirLight[10];
    PointLight gPointLight[10];
    SpotLight gSpotLight[10];
    Material gMaterial;
	int gNumDirLight;
	int gNumPointLight;
	int gNumSpotLight;
    float3 gEyePosW;
}


struct VertexIn
{
	float3 PosL : POSITION;
    float3 NormalL : NORMAL;
	float2 Tex : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosW : POSITION;     // 在世界中的位置
    float3 NormalW : NORMAL;    // 法向量在世界中的方向
	float2 Tex : TEXCOORD;
};










