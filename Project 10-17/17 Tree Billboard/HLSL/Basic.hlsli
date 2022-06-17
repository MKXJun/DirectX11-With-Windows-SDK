#include "LightHelper.hlsli"

Texture2D g_Tex : register(t0);
Texture2DArray g_TexArray : register(t1);
SamplerState g_Sam : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    Material g_Material;
}

cbuffer CBChangesEveryFrame : register(b1)
{
    matrix g_View;
    float3 g_EyePosW;
    float g_Pad;
}

cbuffer CBDrawingStates : register(b2)
{
    float4 g_FogColor;
    int g_FogEnabled;
    float g_FogStart;
    float g_FogRange;
    float g_Pad2;
}

cbuffer CBChangesOnResize : register(b3)
{
    matrix g_Proj;
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

struct VertexPosHWNormalTex
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION; // �������е�λ��
    float3 normalW : NORMAL; // �������������еķ���
    float2 tex : TEXCOORD;
};

struct PointSprite
{
    float3 posW : POSITION;
    float2 SizeW : SIZE;
};

struct BillboardVertex
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};






