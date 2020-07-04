#include "BitonicSort.hlsli"

#define TRANSPOSE_BLOCK_SIZE 16

groupshared uint shared_data[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void CS(uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex)
{
    uint index = DTid.y * g_MatrixWidth + DTid.x;
    shared_data[GI] = g_Input[index];
    GroupMemoryBarrierWithGroupSync();

    uint2 outPos = DTid.yx % g_MatrixHeight + DTid.xy / g_MatrixHeight * g_MatrixHeight;
    g_Data[outPos.y * g_MatrixWidth + outPos.x] = shared_data[GI];
}
