
#ifndef CONSTANT_BUFFERS_HLSL
#define CONSTANT_BUFFERS_HLSL

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_WorldInvTransposeView;
    matrix g_World;
    matrix g_WorldView;
    matrix g_WorldViewProj;
    float4 g_kAlbedo;
    float g_kRoughness;
    float g_kMetallic;
}

cbuffer CBCascadedShadow : register(b1)
{
    matrix g_InvView;
    matrix g_ShadowView;
    matrix g_Proj;
    float4 g_CascadeOffset[8];          // ShadowPT矩阵的平移量
    float4 g_CascadeScale[8];           // ShadowPT矩阵的缩放量
    
    uint2 g_FramebufferDimensions;
    float2 g_CameraNearFar;
    
    // 对Map-based Selection方案，这将保持有效范围内的像素。
    // 当没有边界时，Min和Max分别为0和1
    float  g_MinBorderPadding;          // (kernelSize / 2) / (float)shadowSize
    float  g_MaxBorderPadding;          // 1.0f - (kernelSize / 2) / (float)shadowSize
    int    g_VisualizeCascades;         // 1使用不同的颜色可视化级联阴影，0绘制场景
    float  g_CascadeBlendArea;          // 级联之间重叠量时的混合区域
    
    float  g_TexelSize;                 // Shadow map的纹素大小
    int    g_PCFBlurForLoopStart;       // 循环初始值，5x5的PCF核从-2开始
    int    g_PCFBlurForLoopEnd;         // 循环结束值，5x5的PCF核应该设为3
    float  g_PCFDepthBias;              // 阴影偏移值
    
    float4 g_CascadeFrustumsEyeSpaceDepthsData; // 不同子视锥体远平面的Z值，将级联分开
}

cbuffer CBLight : register(b2)
{
    float3 g_DirLightDir;
    float g_DirLightIntensity;
    float3 g_DirLightColor;
}

#endif // CONSTANTBUFFERS_HLSL
