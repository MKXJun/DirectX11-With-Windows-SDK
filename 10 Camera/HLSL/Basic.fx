#include "LightHelper.hlsli"

Texture2D tex : register(t0);
SamplerState samLinear : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
	row_major matrix gWorld;
	row_major matrix gWorldInvTranspose;
	row_major matrix gTexTransform;
}

cbuffer CBChangesEveryFrame : register(b1)
{
	row_major matrix gView;
	float3 gEyePosW;
}

cbuffer CBChangesOnResize : register(b2)
{
	row_major matrix gProj;
}

cbuffer CBNeverChange : register(b3)
{
	DirectionalLight gDirLight[10];
	PointLight gPointLight[10];
	SpotLight gSpotLight[10];
	Material gMaterial;
	int gNumDirLight;
	int gNumPointLight;
	int gNumSpotLight;
}



struct VertexIn
{
	float3 Pos : POSITION;
    float3 Normal : NORMAL;
	float2 Tex : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosW : POSITION;     // 在世界中的位置
    float3 NormalW : NORMAL;    // 法向量在世界中的方向
	float2 Tex : TEXCOORD;
};




// 顶点着色器(3D)
VertexOut VS_3D(VertexIn pIn)
{
	VertexOut pOut;
    row_major matrix worldViewProj = mul(mul(gWorld, gView), gProj);
    pOut.PosH = mul(float4(pIn.Pos, 1.0f), worldViewProj);     
    pOut.PosW = mul(float4(pIn.Pos, 1.0f), gWorld).xyz;
    pOut.NormalW = mul(pIn.Normal, (float3x3)gWorldInvTranspose);
	pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
    return pOut;
}


// 像素着色器(3D)
float4 PS_3D(VertexOut pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(gEyePosW - pIn.PosW);

    // 初始化为0 
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
	int i;
	// 强制展开循环以减少指令数
	[unroll]
	for (i = 0; i < gNumDirLight; ++i)
	{
		ComputeDirectionalLight(gMaterial, gDirLight[i], pIn.NormalW, toEyeW, A, D, S);
		ambient += A;
		diffuse += D;
		spec += S;
	}
    
	[unroll]
	for (i = 0; i < gNumPointLight; ++i)
	{
		ComputePointLight(gMaterial, gPointLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
		ambient += A;
		diffuse += D;
		spec += S;
	}
    
	[unroll]
	for (i = 0; i < gNumSpotLight; ++i)
	{
		ComputeSpotLight(gMaterial, gSpotLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
		ambient += A;
		diffuse += D;
		spec += S;
	}
    

	float4 texColor = tex.Sample(samLinear, pIn.Tex);
	float4 litColor = texColor * (ambient + diffuse) + spec;
	litColor.a = texColor.a * gMaterial.Diffuse.a;
	
    return litColor;
}

// 顶点着色器(2D)
VertexOut VS_2D(VertexIn pIn)
{
	VertexOut pOut;
	pOut.PosH = float4(pIn.Pos, 1.0f);
	pOut.PosW = float3(0.0f, 0.0f, 0.0f);
	pOut.NormalW = pIn.Normal;
	pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
	return pOut;
}

// 像素着色器(2D)
float4 PS_2D(VertexOut pIn) : SV_Target
{
	return tex.Sample(samLinear, pIn.Tex);
}