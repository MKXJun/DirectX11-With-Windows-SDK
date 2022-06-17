#include "Tessellation.hlsli"

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("TriConstantHS")]
float3 HS(InputPatch<VertexOut, 3> patch, uint i: SV_OutputControlPointID, uint patchId : SV_PrimitiveID) : POSITION
{
    return patch[i].posL;
}
