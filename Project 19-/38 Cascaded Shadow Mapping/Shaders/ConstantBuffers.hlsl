
#ifndef CONSTANTBUFFERS_HLSL
#define CONSTANTBUFFERS_HLSL

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_WorldView;
    matrix g_WorldViewProj;
}

cbuffer CBCascadedShadow : register(b1)
{
    matrix g_ShadowView;            
    float4 g_CascadeOffset[8];      // ShadowPT矩阵的平移量
    float4 g_CascadeScale[8];       // ShadowPT矩阵的缩放量
    int    g_VisualizeCascades;     // 1使用不同的颜色可视化级联阴影，0绘制场景
    int    g_PCFBlurForLoopStart;   // 循环初始值，5x5的PCF核从-2开始
    int    g_PCFBlurForLoopEnd;     // 循环结束值，5x5的PCF核应该设为3
    int    g_Pad;
    
    
    // 对Map-based Selection方案，这将保持有效范围内的像素。
    // 当没有边界时，Min和Max分别为0和1
    float  g_MinBorderPadding;      // (kernelSize / 2) / (float)shadowSize
    float  g_MaxBorderPadding;      // 1.0f - (kernelSize / 2) / (float)shadowSize
    float  g_ShadowBias;            // 处理阴影伪影的偏移值，这些伪影会因为PCF而加剧
    float  g_CascadeBlendArea;      // 级联之间重叠量时的混合区域
    
    
    float  g_TexelSize;             // Shadow map的纹素大小
    float3 g_LightDir;              // 光源方向
    
    float4 g_CascadeFrustumsEyeSpaceDepthsFloat[2]; // 不同子视锥体远平面的Z值，将级联分开
    float4 g_CascadeFrustumsEyeSpaceDepthsFloat4[8];// 以float4花费额外空间的形式使得可以数组遍历，yzw分量无用
}


#endif // CONSTANTBUFFERS_HLSL
