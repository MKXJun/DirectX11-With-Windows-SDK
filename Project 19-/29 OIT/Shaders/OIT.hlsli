cbuffer CBFrame : register(b6)
{
    uint g_FrameWidth;
    uint3 g_Pad3;
}

struct FragmentData
{
    uint color;
    float depth;
};

struct FLStaticNode
{
    FragmentData data;
    uint next;
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

RWStructuredBuffer<FLStaticNode> g_FLBufferRW : register(u1);
RWByteAddressBuffer g_StartOffsetBufferRW : register(u2);

StructuredBuffer<FLStaticNode> g_FLBuffer : register(t2);
ByteAddressBuffer g_StartOffsetBuffer : register(t3);
Texture2D g_BackGround : register(t4);
