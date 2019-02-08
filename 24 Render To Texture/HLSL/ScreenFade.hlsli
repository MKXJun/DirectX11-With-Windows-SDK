
Texture2D gTex : register(t0);
SamplerState gSam : register(s0);

cbuffer CBChangesEveryFrame : register(b0)
{
    float gFadeAmount;      // 颜色程度控制(0.0f-1.0f)
    float3 gPad;
}

cbuffer CBChangesRarely : register(b1)
{
    matrix gWorldViewProj;
}

struct VertexPosTex
{
    float3 PosL : POSITION;
    float2 Tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};