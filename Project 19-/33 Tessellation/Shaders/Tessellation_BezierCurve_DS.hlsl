#include "Tessellation.hlsli"

[domain("isoline")]
float4 DS(IsolinePatchTess patchTess,
    float t : SV_DomainLocation,
    const OutputPatch<HullOut, 4> bezPatch) : SV_POSITION
{
    float4 basisU = BernsteinBasis(t);
    
    // 贝塞尔曲线插值
    float3 sum = basisU.x * bezPatch[0].posL +
        basisU.y * bezPatch[1].posL +
        basisU.z * bezPatch[2].posL +
        basisU.w * bezPatch[3].posL;
    
    float4 posH = mul(float4(sum, 1.0f), g_WorldViewProj);
    
    return posH;
}
