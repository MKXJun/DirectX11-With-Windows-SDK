#include "PostProcess.hlsli"

// 像素着色器
float4 PS(VertexPosHTex pIn) : SV_Target
{
    float2 posW = pIn.tex * float2(g_RectW.zw - g_RectW.xy) + g_RectW.xy;
    
    float4 color = g_Tex.Sample(g_Sam, pIn.tex);
    
    if (length(posW - g_EyePosW.xz) / g_VisibleRange > 1.0f)
    {
        color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return color;
}