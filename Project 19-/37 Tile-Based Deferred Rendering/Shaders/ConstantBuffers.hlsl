
#ifndef CONSTANT_BUFFERS_HLSL
#define CONSTANT_BUFFERS_HLSL

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_WorldInvTransposeView;
    matrix g_WorldView;
    matrix g_InvView;
    matrix g_ViewProj;
    matrix g_Proj;
    matrix g_WorldViewProj;
}

cbuffer CBPerFrame : register(b1)
{
    float4 g_CameraNearFar;   // 不需要反转 
    uint4 g_FramebufferDimensions;
    uint g_LightingOnly;
    uint g_FaceNormals;
    uint g_VisualizeLightCount;
    uint g_VisualizePerSampleShading;
}

#endif // CONSTANTBUFFERS_HLSL
