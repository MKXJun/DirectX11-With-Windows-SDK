
#ifndef CONSTANTBUFFERS_HLSL
#define CONSTANTBUFFERS_HLSL

cbuffer CBChangesEveryInstanceDrawing : register(b0)
{
    matrix g_World;
    matrix g_WorldInvTranspose;
    matrix g_WorldViewProj;
}

cbuffer CBChangesEveryObjectDrawing : register(b1)
{
    float4 g_Kspecular;
}

cbuffer CBPerFrame : register(b2)
{
    matrix g_ShadowTransform; // ShadowView * ShadowProj * T
    
    float3 g_LightPosW;
    float g_LightIntensity;
    
    float3 g_CameraPosW;
}

#endif // CONSTANTBUFFERS_HLSL
