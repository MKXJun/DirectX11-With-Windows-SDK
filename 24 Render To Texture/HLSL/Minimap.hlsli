
Texture2D tex : register(t0);
SamplerState sam : register(s0);

cbuffer CBChangesEveryFrame : register(b0)
{
    float3 gEyePosW;            // 摄像机位置
    float gPad;
}

cbuffer CBDrawingStates : register(b1)
{
    int gFogEnabled;            // 是否范围可视
    float gVisibleRange;        // 3D世界可视范围
    float2 gPad2;
    float4 gRectW;              // 小地图xOz平面对应3D世界矩形区域(Left, Front, Right, Back)
    float4 gInvisibleColor;     // 不可视情况下的颜色
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





