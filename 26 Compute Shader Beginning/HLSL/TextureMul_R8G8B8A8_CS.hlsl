Texture2D gTexA : register(t0);
Texture2D gTexB : register(t1);

RWTexture2D<unorm float4> gOutput : register(u0);

// 一个线程组中的线程数目。线程可以1维展开，也可以
// 2维或3维排布
[numthreads(16, 16, 1)]
void CS( uint3 DTid : SV_DispatchThreadID )
{
    gOutput[DTid.xy] = (unorm float4)(gTexA[DTid.xy] * gTexB[DTid.xy]);
}
