
Texture2D g_Tex : register(t0);
SamplerState g_Sam : register(s0);

cbuffer CBChangesEveryFrame : register(b0)
{
    float3 g_EyePosW;            // 摄像机位置
    float g_Pad;
}

cbuffer CBDrawingStates : register(b1)
{
    int g_FogEnabled;            // 是否范围可视
    float g_VisibleRange;        // 3D世界可视范围
    float2 g_Pad2;
    float4 g_RectW;              // 小地图xOz平面对应3D世界矩形区域(Left, Front, Right, Back)
    float4 g_InvisibleColor;     // 不可视情况下的颜色
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





