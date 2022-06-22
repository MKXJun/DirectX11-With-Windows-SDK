
Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer CB : register(b0)
{
    float g_VisibleRange;        // 3D世界可视范围
    float3 g_EyePosW;            // 摄像机位置
    float4 g_RectW;              // 小地图xOz平面对应3D世界矩形区域(Left, Front, Right, Back)
}

struct VertexPosTex
{
    float3 posL : POSITION;
    float2 tex : TEXCOORD;
};

struct VertexPosHTex
{
    float4 posH : SV_POSITION;
    float2 tex : TEXCOORD;
};
