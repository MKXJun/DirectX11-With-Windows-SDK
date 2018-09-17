#include "LightHelper.hlsli"

Texture2D tex : register(t0);
Texture2DArray texArray : register(t1);
SamplerState sam : register(s0);


cbuffer CBChangesEveryDrawing : register(b0)
{
    row_major matrix gWorld;
    row_major matrix gWorldInvTranspose;
    row_major matrix gTexTransform;
    Material gMaterial;
}

cbuffer CBChangesEveryFrame : register(b1)
{
    row_major matrix gView;
    float3 gEyePosW;
}

cbuffer CBDrawingStates : register(b2)
{
    float4 gFogColor;
    int gFogEnabled;
    float gFogStart;
    float gFogRange;
}

cbuffer CBChangesOnResize : register(b3)
{
    row_major matrix gProj;
}

cbuffer CBChangesRarely : register(b4)
{
    DirectionalLight gDirLight[4];
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
    float3 PosW : POSITION; // 在世界中的位置
    float3 NormalW : NORMAL; // 法向量在世界中的方向
    float2 Tex : TEXCOORD;
};

struct PointSprite
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

struct BillboardVertex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};






