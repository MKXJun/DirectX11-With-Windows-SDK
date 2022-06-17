#include "Tessellation.hlsli"

// 0  1
// .__.
//   /
// ./_.
// 2  3 
[domain("quad")]
[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("QuadConstantHS")]
[maxtessfactor(64.0f)]
float3 HS(InputPatch<VertexOut, 4> patch, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID) : POSITION
{
    return patch[i].posL;
}
