
Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer CBChangesEveryFrame : register(b0)
{
    float g_FadeAmount;      // 颜色程度控制(0.0f-1.0f)
    float3 g_Pad;
}

cbuffer CBChangesRarely : register(b1)
{
    matrix g_WorldViewProj;
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