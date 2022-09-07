
#ifndef COMPUTE_SHADER_TILE_HLSL
#define COMPUTE_SHADER_TILE_HLSL

#define MAX_LIGHTS_POWER 10
#define MAX_LIGHTS (1 << MAX_LIGHTS_POWER)
#define MAX_LIGHT_INDICES ((MAX_LIGHTS >> 3) - 1)

// 确定分块(tile)的大小用于光照去除和相关的权衡取舍 
#define COMPUTE_SHADER_TILE_GROUP_DIM 16
#define COMPUTE_SHADER_TILE_GROUP_SIZE (COMPUTE_SHADER_TILE_GROUP_DIM*COMPUTE_SHADER_TILE_GROUP_DIM)


#include "GBuffer.hlsl"

RWTexture2D<float4> g_FrameBuffer : register(u0);

groupshared uint gs_MinZ;
groupshared uint gs_MaxZ;

// 当前tile的光照列表
groupshared uint gs_TileLightIndices[MAX_LIGHTS >> 3];
groupshared uint gs_TileNumLights;

// 当前tile中需要逐样本着色的像素列表
// 我们将两个16位x/y坐标编码进一个uint来节省共享内存空间
groupshared uint gs_PerSamplePixels[COMPUTE_SHADER_TILE_GROUP_SIZE];
groupshared uint gs_NumPerSamplePixels;

void ConstructFrustumPlanes(uint3 groupId, float minTileZ, float maxTileZ,
                            out float4 frustumPlanes[6])
{
    // 注意：下面的计算每个分块都是统一的(例如：不需要每个线程都执行)，但代价低廉。
    // 我们可以只是先为每个分块预计算视锥平面，然后将结果放到一个常量缓冲区中...
    // 只有当投影矩阵改变的时候才需要变化，因为我们是在观察空间执行，
    // 然后我们就只需要计算近/远平面来贴紧我们实际的几何体。
    // 不管怎样，组同步/局部数据共享(Local Data Share, LDS)或全局内存寻找的开销可能和这小段数学一样多，但值得尝试。
    
    // 原Intel样例程序计算的Scale和Bias有误，这里重新推导了一遍
    float2 tileScale = float2(g_FramebufferDimensions.xy) / COMPUTE_SHADER_TILE_GROUP_DIM;
    float2 tileBias = tileScale - 1.0f - 2.0f * float2(groupId.xy);

    // 计算当前分块视锥体的投影矩阵
    float4 c1 = float4(g_Proj._11 * tileScale.x, 0.0f, tileBias.x, 0.0f);
    float4 c2 = float4(0.0f, g_Proj._22 * tileScale.y, -tileBias.y, 0.0f);
    float4 c4 = float4(0.0f, 0.0f, 1.0f, 0.0f);

    // Gribb/Hartmann法提取视锥体平面
    // 侧面
    frustumPlanes[0] = c4 - c1; // 右裁剪平面 
    frustumPlanes[1] = c4 + c1; // 左裁剪平面
    frustumPlanes[2] = c4 - c2; // 上裁剪平面
    frustumPlanes[3] = c4 + c2; // 下裁剪平面
    // 近/远平面
    frustumPlanes[4] = float4(0.0f, 0.0f, 1.0f, -minTileZ);
    frustumPlanes[5] = float4(0.0f, 0.0f, -1.0f, maxTileZ);
    
    // 标准化视锥体平面(近/远平面已经标准化)
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
    }
}

[numthreads(COMPUTE_SHADER_TILE_GROUP_DIM, COMPUTE_SHADER_TILE_GROUP_DIM, 1)]
void ComputeShaderTileDeferredCS(uint3 groupId : SV_GroupID,
                                 uint3 dispatchThreadId : SV_DispatchThreadID,
                                 uint3 groupThreadId : SV_GroupThreadID,
                                 uint groupIndex : SV_GroupIndex
                                 )
{
    //
    // 获取表面数据，计算当前分块的视锥体
    //
    
    uint2 globalCoords = dispatchThreadId.xy;
    
    SurfaceData surfaceData = ComputeSurfaceDataFromGBuffer(globalCoords);
    
    // 寻找所有采样中的Z边界
    float minZSample = g_CameraNearFar.y;
    float maxZSample = g_CameraNearFar.x;
    // 避免对天空盒或其它非法像素着色
    float viewSpaceZ = surfaceData.posV.z;
    bool validPixel = viewSpaceZ >= g_CameraNearFar.x && viewSpaceZ < g_CameraNearFar.y;
        [flatten]
    if (validPixel)
    {
        minZSample = min(minZSample, viewSpaceZ);
        maxZSample = max(maxZSample, viewSpaceZ);
    }
    
    // 初始化共享内存中的光照列表和Z边界
    if (groupIndex == 0)
    {
        gs_TileNumLights = 0;
        gs_NumPerSamplePixels = 0;
        gs_MinZ = 0x7F7FFFFF; // 最大浮点数
        gs_MaxZ = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    // 注意：这里可以进行并行归约(parallel reduction)的优化，但由于我们使用了MSAA并
    // 存储了多重采样的像素在共享内存中，逐渐增加的共享内存压力实际上**减小**内核的总
    // 体运行速度。因为即便是在最好的情况下，在目前具有典型分块(tile)大小的的架构上，
    // 并行归约的速度优势也是不大的。
    // 只有少量实际合法样本的像素在其中。
    if (maxZSample >= minZSample)
    {
        InterlockedMin(gs_MinZ, asuint(minZSample));
        InterlockedMax(gs_MaxZ, asuint(maxZSample));
    }

    GroupMemoryBarrierWithGroupSync();
    
    float minTileZ = asfloat(gs_MinZ);
    float maxTileZ = asfloat(gs_MaxZ);
    float4 frustumPlanes[6];
    ConstructFrustumPlanes(groupId, minTileZ, maxTileZ, frustumPlanes);
    
    //
    // 对当前分块(tile)进行光照裁剪
    //
 
    uint totalLights, dummy;
    g_Light.GetDimensions(totalLights, dummy);

    // 组内每个线程承担一部分光源的碰撞检测计算
    for (uint lightIndex = groupIndex; lightIndex < totalLights; lightIndex += COMPUTE_SHADER_TILE_GROUP_SIZE)
    {
        PointLight light = g_Light[lightIndex];
                
        // 点光源球体与tile视锥体的碰撞检测
        bool inFrustum = true;
        [unroll]
        for (uint i = 0; i < 6; ++i)
        {
            float d = dot(frustumPlanes[i], float4(light.posV, 1.0f));
            inFrustum = inFrustum && (d >= -light.range);
        }

        [branch]
        if (inFrustum)
        {
            // 将光照追加到列表中
            uint listIndex;
            InterlockedAdd(gs_TileNumLights, 1, listIndex);
            gs_TileLightIndices[listIndex] = lightIndex;
        }
    }

    GroupMemoryBarrierWithGroupSync();
    
    //
    // 只处理在屏幕区域的像素(单个分块可能超出屏幕边缘)
    // 
    if (all(globalCoords < g_FramebufferDimensions.xy))
    {
        
        float3 lit = CalcDirectionalLight(surfaceData, g_DirLightDir.xyz, g_DirLightColor.rgb, g_DirLightIntensity) 
                        * surfaceData.albedo.w;

        uint numLights = gs_TileNumLights;
        [branch]
        if (numLights > 0)
        {
            for (uint tileLightIndex = 0; tileLightIndex < numLights; ++tileLightIndex)
            {
                PointLight light = g_Light[gs_TileLightIndices[tileLightIndex]];
                lit += CalcPointLight(surfaceData, light);
            }
        }
        
        lit *= surfaceData.ambientOcclusion;
        
        // 计算结果
        g_FrameBuffer[globalCoords] = float4(lit, 1.0f);
    }
}

#endif // COMPUTE_SHADER_TILE_HLSL
