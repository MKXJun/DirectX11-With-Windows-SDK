cbuffer CBFrame : register(b6)
{
    uint g_FrameWidth;
    uint g_FrameHeight;
    uint2 g_Pad2;
}

struct FragmentData
{
    uint Color;
    float Depth;
};

struct FLStaticNode
{
    FragmentData Data;
    uint Next;
};

uint PackColorFromFloat4(float4 color)
{
    uint4 colorUInt4 = (uint4) (color * 255.0f);
    return colorUInt4.r | (colorUInt4.g << 8) | (colorUInt4.b << 16) | (colorUInt4.a << 24);
}

float4 UnpackColorFromUInt(uint color)
{
    uint4 colorUInt4 = uint4(color, color >> 8, color >> 16, color >> 24) & (0x000000FF);
    return (float4) colorUInt4 / 255.0f;
}