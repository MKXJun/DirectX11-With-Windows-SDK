#include "Tessellation.hlsli"

[domain("isoline")]
[partitioning("integer")]
[outputtopology("line")]
[outputcontrolpoints(4)]
[patchconstantfunc("IsolineConstantHS")]
[maxtessfactor(64.0f)]
float3 HS(InputPatch<VertexOut, 4> patch, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID) : POSITION
{
    return patch[i].posL;
}
