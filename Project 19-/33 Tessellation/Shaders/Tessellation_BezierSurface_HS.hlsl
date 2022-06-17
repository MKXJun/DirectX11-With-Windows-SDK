#include "Tessellation.hlsli"

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("QuadPatchConstantHS")]
[maxtessfactor(64.0f)]
float3 HS(InputPatch<VertexOut, 16> patch, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID) : POSITION
{
    return patch[i].posL;
}
