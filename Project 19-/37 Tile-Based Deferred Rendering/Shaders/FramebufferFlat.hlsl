
#ifndef FRAMEBUFFER_FLAT_HLSL
#define FRAMEBUFFER_FLAT_HLSL

// 将R16G16B16A16_UNORM解包为float4
float4 UnpackRGBA16(uint2 e)
{
    return float4(f16tof32(e), f16tof32(e >> 16));
}

// 将float4打包为R16G16B16A16_UNORM
uint2 PackRGBA16(float4 c)
{
    return f32tof16(c.rg) | (f32tof16(c.ba) << 16);
}

// 根据给定的2D地址和采样索引定位到我们的1D帧缓冲数组
uint GetFramebufferSampleAddress(uint2 coords, uint sampleIndex)
{
    // 行主序: Row (x), Col (y), MSAA sample
    return (sampleIndex * g_FramebufferDimensions.y + coords.y) * g_FramebufferDimensions.x + coords.x;
}

#endif // FRAMEBUFFER_FLAT_HLSL
