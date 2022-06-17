#include "LightHelper.hlsli"

Texture2D g_DiffuseMap : register(t0);
Texture2D g_NormalMap : register(t1);
TextureCube g_TexCube : register(t2);
SamplerState g_Sam : register(s0);


cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
}

cbuffer CBChangesEveryObjectDrawing : register(b1)
{
    Material g_Material;
}

cbuffer CBDrawingStates : register(b2)
{
    int g_ReflectionEnabled;
    int g_RefractionEnabled;
    float g_Eta; // 空气/介质折射比
    int g_Pad;
}

cbuffer CBChangesEveryFrame : register(b3)
{
    matrix g_ViewProj;
    float3 g_EyePosW;
    float g_Pad2;
}

cbuffer CBChangesRarely : register(b4)
{
    DirectionalLight g_DirLight[5];
    PointLight g_PointLight[5];
    SpotLight g_SpotLight[5];
}

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 tex : TEXCOORD;
};

struct VertexPosNormalTangentTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float4 tangentL : TANGENT;
    float2 tex : TEXCOORD;
};

struct InstancePosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 tex : TEXCOORD;
    matrix world : World;
    matrix worldInvTranspose : WorldInvTranspose;
};

struct InstancePosNormalTangentTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float4 tangentL : TANGENT;
    float2 tex : TEXCOORD;
    matrix world : World;
    matrix worldInvTranspose : WorldInvTranspose;
};

struct VertexPosHWNormalTex
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION; // 在世界中的位置
    float3 normalW : NORMAL; // 法向量在世界中的方向
    float2 tex : TEXCOORD;
};

struct VertexPosHWNormalTangentTex
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION; // 在世界中的位置
    float3 normalW : NORMAL; // 法向量在世界中的方向
    float4 tangentW : TANGENT; // 切线在世界中的方向
    float2 tex : TEXCOORD;
};

