
cbuffer CB : register(b0)
{
    matrix g_WorldViewProj;
    matrix g_ViewProj;
}

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

VertexPosHTex VS(VertexPosNormalTex vIn)
{
    VertexPosHTex vOut;

    vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_WorldViewProj);
    vOut.Tex = vIn.Tex;

    return vOut;
}
