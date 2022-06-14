#include "PostProcess.hlsli"

float4 PS(float4 posH : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
    float4 c = g_Input.SampleLevel(g_SamPointClamp, texcoord, 0.0f);
    uint texWidth, texHeight;
    g_EdgeInput.GetDimensions(texWidth, texHeight);
    float4 e = float4(1.0f, 1.0f, 1.0f, 1.0f);
    if (texWidth > 0 && texHeight > 0)
    {
        e = g_EdgeInput.SampleLevel(g_SamPointClamp, texcoord, 0.0f);
    }
    // 将原始图片与边缘图相乘
    return c * e;
}
