#include "Minimap.hlsli"

// 像素着色器
float4 PS(VertexPosHTex pIn) : SV_Target
{
    // 要求Tex的取值范围都在[0.0f, 1.0f], y值对应世界坐标z轴
    float2 PosW = pIn.Tex * float2(g_RectW.zw - g_RectW.xy) + g_RectW.xy;
    
    float4 color = g_Tex.Sample(g_Sam, pIn.Tex);

    [flatten]
    if (g_FogEnabled && length(PosW - g_EyePosW.xz) / g_VisibleRange > 1.0f)
    {
        return g_InvisibleColor;
    }

    return color;
}