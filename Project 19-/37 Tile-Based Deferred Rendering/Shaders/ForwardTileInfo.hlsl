
#ifndef FORWARD_TILE_INFO_HLSL
#define FORWARD_TILE_INFO_HLSL

#include "ShaderDefines.h"

//
// Forward+ Tile-Based
//
struct TileInfo
{
    uint tileNumLights;
    uint tileLightIndices[MAX_LIGHT_INDICES];
};

#endif // FORWARD_TILE_INFO_HLSL
