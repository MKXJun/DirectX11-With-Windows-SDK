Texture2D g_DiffuseMap : register(t0);
SamplerState g_Sam : register(s0);

uniform matrix g_WorldViewProj;

struct VertexPosNormalTex
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

