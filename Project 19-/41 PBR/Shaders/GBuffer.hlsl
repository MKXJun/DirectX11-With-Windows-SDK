
#ifndef GBUFFER_HLSL
#define GBUFFER_HLSL

#include "Rendering.hlsl"

//--------------------------------------------------------------------------------------
// GBuffer、相关常用工具和结构
struct GBuffer
{
    float4 normalRoughness : SV_Target0; // RGBA8_UNORM : 24 normal | 8 roughness
    float4 albedoMetallic : SV_Target1;    // RGBA8_UNORM : 24 RGB | 8 metallic
};

// 上述GBuffer加上深度缓冲区(最后一个元素)  t11-t13
Texture2D<float4> g_GBufferTextures[3] : register(t11);

float3 ComputePositionViewFromZ(float2 posNdc, float viewSpaceZ)
{
    float2 screenSpaceRay = float2(posNdc.x / g_Proj._m00,
                                   posNdc.y / g_Proj._m11);
    
    float3 posV;
    posV.z = viewSpaceZ;
    posV.xy = screenSpaceRay.xy * posV.z;
    
    return posV;
}

SurfaceData ComputeSurfaceDataFromGeometry(VertexOut v)
{
    SurfaceData surfaceData = (SurfaceData) 0.0f;
    
    surfaceData.posV = v.posV;
    surfaceData.albedo = g_kAlbedo * g_AlbedoMap.Sample(g_Sam, v.texCoord);

#if defined(USE_NORMAL_MAP)
    float3 normalSample = g_NormalMap.Sample(g_Sam, v.texCoord).xyz;
#if defined(USE_TANGENT)
    surfaceData.normalV = TransformNormalSample(normalSample, v.normalV, v.tangentV);
#else
    surfaceData.normalV = TransformNormalSample(normalSample, v.normalV, v.posV, v.texCoord);
#endif
#else
    surfaceData.normalV = normalize(v.normalV);
#endif
    
#if defined(USE_MIXED_ARM_MAP)
    float3 rm = g_RoughnessMetallicMap.Sample(g_Sam, v.texCoord).rgb;
    surfaceData.ambientOcclusion = 1.0f;
    surfaceData.roughness = g_kRoughness * rm.g;
    surfaceData.metallic = g_kMetallic * rm.b;
#else
    surfaceData.roughness = g_kRoughness * g_RoughnessMap.Sample(g_Sam, v.texCoord).r;
    surfaceData.metallic = g_kMetallic * g_MetallicMap.Sample(g_Sam, v.texCoord).r;
#endif
    
    return surfaceData;
}

SurfaceData ComputeSurfaceDataFromGBuffer(uint2 posViewport)
{
    // 从GBuffer读取数据
    GBuffer rawData;
    rawData.normalRoughness = g_GBufferTextures[0][posViewport.xy];
    rawData.albedoMetallic = g_GBufferTextures[1][posViewport.xy];
    float zBuffer = g_GBufferTextures[2][posViewport.xy].x;
    
    float2 gbufferDim;
    g_GBufferTextures[0].GetDimensions(gbufferDim.x, gbufferDim.y);
    
    // 计算屏幕/裁剪空间坐标和相邻的位置
    // 注意：需要留意DX11的视口变换和像素中心位于(x+0.5, y+0.5)位置
    // 注意：该偏移实际上可以在CPU预计算但将它放到常量缓冲区读取实际上比在这里重新计算更慢一些
    float2 screenPixelOffset = float2(2.0f, -2.0f) / gbufferDim;
    float2 posNdc = (float2(posViewport.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    float2 posNdcX = posNdc + float2(screenPixelOffset.x, 0.0f);
    float2 posNdcY = posNdc + float2(0.0f, screenPixelOffset.y);
        
    // 解码到合适的输出
    SurfaceData surfaceData = (SurfaceData)0.0f;
        
    // 反投影深度缓冲Z值到观察空间
    float viewSpaceZ = g_Proj._m32 / (zBuffer - g_Proj._m22);

    surfaceData.posV = ComputePositionViewFromZ(posNdc, viewSpaceZ);
    surfaceData.normalV = rawData.normalRoughness.xyz;
    surfaceData.roughness = rawData.normalRoughness.w;
    surfaceData.metallic = rawData.albedoMetallic.w;
    //surfaceData.normalV = rawData.normalRM.xyz;
    //surfaceData.roughness = rawData.normalRM.w;
    //surfaceData.metallic = rawData.albedoAO.w;
    surfaceData.ambientOcclusion = 1.0f;
    
    float r = surfaceData.roughness + 1.0f;
    surfaceData.k_direct = r * r / 8.0f;
    
    // 大多数非金属的F0范围为0.02-0.05(线性值)
    static const float specularCoefficient = 0.04f;
    surfaceData.F0 = lerp(specularCoefficient, surfaceData.albedo.rgb, surfaceData.metallic);
    
    surfaceData.albedo.xyz = rawData.albedoMetallic.rgb;
    
    // 阴影
    float3 posW = mul(float4(surfaceData.posV, 1.0f), g_InvView).xyz;
    float4 shadowView = mul(float4(posW, 1.0f), g_ShadowView);
    int cascadeIndex = 0, nextCascadeIndex = 0;
    float blendWeight = 0.0f;
    surfaceData.albedo.w = CalculateCascadedShadow(shadowView, viewSpaceZ, cascadeIndex, nextCascadeIndex, blendWeight);
    
    return surfaceData;
}

//--------------------------------------------------------------------------------------
// G-buffer 渲染
//--------------------------------------------------------------------------------------
void GBufferPS(VertexOut input, uniform bool alphaClip, out GBuffer outputGBuffer)
{
    SurfaceData surface = ComputeSurfaceDataFromGeometry(input);
    if (alphaClip)
    {
        clip(surface.albedo.a - 0.05f);
    }
    
    outputGBuffer.normalRoughness = float4(surface.normalV, surface.roughness);
    outputGBuffer.albedoMetallic = float4(surface.albedo.rgb, surface.metallic);
}

//--------------------------------------------------------------------------------------
// 调试
//--------------------------------------------------------------------------------------
float4 DebugNormalPS(float4 posViewport : SV_Position) : SV_Target
{
    float3 normalV = g_GBufferTextures[0][posViewport.xy].xyz;
    float3 normalW = mul(normalV, (float3x3) g_InvView);
    // [-1, 1] => [0, 1]
    return float4((normalW + 1.0f) / 2.0f, 1.0f);
}

float4 DebugAlbedoPS(float4 posViewport : SV_Position) : SV_Target
{
    float3 albedo = g_GBufferTextures[1][posViewport.xy].rgb;
    return float4(albedo, 1.0f);
}

float4 DebugRoughnessPS(float4 posViewport : SV_Position) : SV_Target
{
    float roughness = g_GBufferTextures[0][posViewport.xy].w;
    return float4(roughness.rrr, 1.0f);
}

float4 DebugMetallicPS(float4 posViewport : SV_Position) : SV_Target
{
    float metallic = g_GBufferTextures[1][posViewport.xy].w;
    return float4(metallic.rrr, 1.0f);
}



#endif // GBUFFER_HLSL
