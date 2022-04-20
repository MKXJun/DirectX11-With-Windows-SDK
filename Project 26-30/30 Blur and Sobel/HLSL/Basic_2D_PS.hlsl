#include "Basic.hlsli"

// 像素着色器(2D)
float4 PS(VertexPosHTex pIn) : SV_Target
{
    uint texWidth, texHeight;
    g_DiffuseMap.GetDimensions(texWidth, texHeight);
    float4 texColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (texWidth > 0 && texHeight > 0)
    {
        texColor = g_DiffuseMap.Sample(g_SamLinearWrap, pIn.Tex);
    }
    return texColor;
}