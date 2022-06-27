#include "PostProcess.hlsli"

float4 PS(float4 posH : SV_position, float2 texcoord : TEXCOORD) : SV_Target
{
    float4 c = g_Input.SampleLevel(g_SamPointClamp, texcoord, 0.0f);
    float4 e = g_EdgeInput.SampleLevel(g_SamPointClamp, texcoord, 0.0f);
    // 将原始图片与边缘图相乘
    return c * e;
}
