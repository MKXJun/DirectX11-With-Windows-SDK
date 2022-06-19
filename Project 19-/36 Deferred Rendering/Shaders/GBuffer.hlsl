
#ifndef GBUFFER_HLSL
#define GBUFFER_HLSL

#include "Rendering.hlsl"

#ifndef MSAA_SAMPLES
#define MSAA_SAMPLES 1
#endif

//--------------------------------------------------------------------------------------
// GBuffer、相关常用工具和结构
struct GBuffer
{
    float4 normal_specular : SV_Target0;
    float4 albedo : SV_Target1;
    float2 posZGrad : SV_Target2;  // ( d(x+1,y)-d(x,y), d(x,y+1)-d(x,y) )
};

// 上述GBuffer加上深度缓冲区(最后一个元素)  t1-t4
Texture2DMS<float4, MSAA_SAMPLES> g_GBufferTextures[4] : register(t1);

// 法线编码
float2 EncodeSphereMap(float3 normal)
{
    return normalize(normal.xy) * (sqrt(-normal.z * 0.5f + 0.5f));
}

// 法线解码
float3 DecodeSphereMap(float2 encoded)
{
    float4 nn = float4(encoded, 1, -1);
    float l = dot(nn.xyz, -nn.xyw);
    nn.z = l;
    nn.xy *= sqrt(l);
    return nn.xyz * 2 + float3(0, 0, -1);
}

float3 ComputePositionViewFromZ(float2 posNdc, float viewSpaceZ)
{
    float2 screenSpaceRay = float2(posNdc.x / g_Proj._m00,
                                   posNdc.y / g_Proj._m11);
    
    float3 posV;
    posV.z = viewSpaceZ;
    posV.xy = screenSpaceRay.xy * posV.z;
    
    return posV;
}

SurfaceData ComputeSurfaceDataFromGBufferSample(uint2 posViewport, uint sampleIndex)
{
    // 从GBuffer读取数据
    GBuffer rawData;
    rawData.normal_specular = g_GBufferTextures[0].Load(posViewport.xy, sampleIndex).xyzw;
    rawData.albedo = g_GBufferTextures[1].Load(posViewport.xy, sampleIndex).xyzw;
    rawData.posZGrad = g_GBufferTextures[2].Load(posViewport.xy, sampleIndex).xy;
    float zBuffer = g_GBufferTextures[3].Load(posViewport.xy, sampleIndex).x;
    
    float2 gbufferDim;
    uint dummy;
    g_GBufferTextures[0].GetDimensions(gbufferDim.x, gbufferDim.y, dummy);
    
    // 计算屏幕/裁剪空间坐标和相邻的位置
    // 注意：需要留意DX11的视口变换和像素中心位于(x+0.5, y+0.5)位置
    // 注意：该偏移实际上可以在CPU预计算但将它放到常量缓冲区读取实际上比在这里重新计算更慢一些
    float2 screenPixelOffset = float2(2.0f, -2.0f) / gbufferDim;
    float2 posNdc = (float2(posViewport.xy) + 0.5f) * screenPixelOffset.xy + float2(-1.0f, 1.0f);
    float2 posNdcX = posNdc + float2(screenPixelOffset.x, 0.0f);
    float2 posNdcY = posNdc + float2(0.0f, screenPixelOffset.y);
        
    // 解码到合适的输出
    SurfaceData data;
        
    // 反投影深度缓冲Z值到观察空间
    float viewSpaceZ = g_Proj._m32 / (zBuffer - g_Proj._m22);

    data.posV = ComputePositionViewFromZ(posNdc, viewSpaceZ);
    data.posV_DX = ComputePositionViewFromZ(posNdcX, viewSpaceZ + rawData.posZGrad.x) - data.posV;
    data.posV_DY = ComputePositionViewFromZ(posNdcY, viewSpaceZ + rawData.posZGrad.y) - data.posV;

    data.normalV = DecodeSphereMap(rawData.normal_specular.xy);
    data.albedo = rawData.albedo;

    data.specularAmount = rawData.normal_specular.z;
    data.specularPower = rawData.normal_specular.w;
    
    return data;
}

void ComputeSurfaceDataFromGBufferAllSamples(uint2 posViewport,
                                             out SurfaceData surface[MSAA_SAMPLES])
{
    // 为每个采样读取数据
    // 大多数时候只有少量数据会被使用，所以展开循环确保编译器容易进行无用代码删除
    [unroll]
    for (uint i = 0; i < MSAA_SAMPLES; ++i)
    {
        surface[i] = ComputeSurfaceDataFromGBufferSample(posViewport, i);
    }
}

// 检查一个给定的像素是否需要进行逐样本着色
bool RequiresPerSampleShading(SurfaceData surface[MSAA_SAMPLES])
{
    bool perSample = false;

    const float maxZDelta = abs(surface[0].posV_DX.z) + abs(surface[0].posV_DY.z);
    const float minNormalDot = 0.99f; // 允许大约8度的法线角度差异

    [unroll]
    for (uint i = 1; i < MSAA_SAMPLES; ++i)
    {
        // 使用三角形的位置偏移，如果所有的采样深度存在差异较大的情况，则有可能属于边界
        perSample = perSample ||
            abs(surface[i].posV.z - surface[0].posV.z) > maxZDelta;

        // 若法线角度差异较大，则有可能来自不同的三角形/表面
        perSample = perSample ||
            dot(surface[i].normalV, surface[0].normalV) < minNormalDot;
    }

    return perSample;
}

// 使用逐采样(1)/逐像素(0)标志来初始化模板掩码值
void RequiresPerSampleShadingPS(float4 posViewport : SV_Position)
{
    SurfaceData surfaceSamples[MSAA_SAMPLES];
    ComputeSurfaceDataFromGBufferAllSamples(uint2(posViewport.xy), surfaceSamples);
    bool perSample = RequiresPerSampleShading(surfaceSamples);

    // 如果我们不需要逐采样着色，抛弃该像素片元(例如：不写入模板)
    [flatten]
    if (!perSample)
    {
        discard;
    }
}

//--------------------------------------------------------------------------------------
// G-buffer 渲染
//--------------------------------------------------------------------------------------
void GBufferPS(VertexPosHVNormalVTex input, out GBuffer outputGBuffer)
{
    SurfaceData surface = ComputeSurfaceDataFromGeometry(input);
    outputGBuffer.normal_specular = float4(EncodeSphereMap(surface.normalV),
                                           surface.specularAmount,
                                           surface.specularPower);
    outputGBuffer.albedo = surface.albedo;
    outputGBuffer.posZGrad = float2(ddx_coarse(surface.posV.z),
                                    ddy_coarse(surface.posV.z));
}

//--------------------------------------------------------------------------------------
// 调试生成法线贴图
//--------------------------------------------------------------------------------------
float4 DebugNormalPS(float4 posViewport : SV_Position) : SV_Target
{
    float4 normal_specular = g_GBufferTextures[0].Load(posViewport.xy, 0).xyzw;
    float3 normalV = DecodeSphereMap(normal_specular.xy);
    float3 normalW = mul(float4(normalV, 0.0f), g_InvView).xyz;
    // 由于渲染目标为sRGB，这里去除伽马校正来观察原始法线贴图
    // [-1, 1] => [0, 1]
    return pow(float4((normalW + 1.0f) / 2.0f, 1.0f), 2.2f);
}

//--------------------------------------------------------------------------------------
// 调试深度值梯度贴图
//--------------------------------------------------------------------------------------
float4 DebugPosZGradPS(float4 posViewport : SV_Position) : SV_Target
{
    float2 posZGrad = g_GBufferTextures[2].Load(posViewport.xy, 0).xy;
    // 由于渲染目标为sRGB，这里去除伽马校正来观察深度值梯度贴图
    return pow(float4(posZGrad, 0.0f, 1.0f), 2.2f);
}

#endif // GBUFFER_HLSL
