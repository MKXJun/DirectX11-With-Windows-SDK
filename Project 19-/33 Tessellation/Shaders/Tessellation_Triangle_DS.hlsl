#include "Tessellation.hlsli"

[domain("tri")]
float4 DS(TriPatchTess patchTess,
    float3 weights : SV_DomainLocation,
    const OutputPatch<HullOut, 3> tri) : SV_POSITION
{
    // 重心坐标系插值
    float3 pos = tri[0].posL * weights[0] +
        tri[1].posL * weights[1] +
        tri[2].posL * weights[2];
    
    return float4(pos, 1.0f);
}
