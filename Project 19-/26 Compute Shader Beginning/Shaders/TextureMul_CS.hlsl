Texture2D<float4> g_TexA : register(t0);
Texture2D<float4> g_TexB : register(t1);

// DXGI_FORMAT                     |  RWTexture2D<T>
// --------------------------------+------------------
// DXGI_FORMAT_R8G8B8A8_UNORM      |  unorm float4
// DXGI_FORMAT_R16G16B16A16_UNORM  |  unorm float4
// DXGI_FORMAT_R8G8B8A8_SNORM      |  snorm float4
// DXGI_FORMAT_R16G16B16A16_SNORM  |  snorm float4
// DXGI_FORMAT_R16G16B16A16_FLOAT  |  float4 或 half4?
// DXGI_FORMAT_R32G32B32A32_FLOAT  |  float4
RWTexture2D<unorm float4> g_Output : register(u0);

// 一个线程组中的线程数目。线程可以1维展开，也可以
// 2维或3维排布
[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    g_Output[DTid.xy] = g_TexA[DTid.xy] * g_TexB[DTid.xy];
}
