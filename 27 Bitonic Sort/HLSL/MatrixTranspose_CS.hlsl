#include "BitonicSort.hlsli"

#define TRANSPOSE_BLOCK_SIZE 16

groupshared uint shared_data[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void CS(uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex)
{
    uint index = DTid.y * gMatrixWidth + DTid.x;
    shared_data[GI] = gInput[index];
    GroupMemoryBarrierWithGroupSync();

    uint2 outPos = DTid.yx % gMatrixHeight + DTid.xy / gMatrixHeight * gMatrixHeight;
    gData[outPos.y * gMatrixWidth + outPos.x] = shared_data[GI];
}
