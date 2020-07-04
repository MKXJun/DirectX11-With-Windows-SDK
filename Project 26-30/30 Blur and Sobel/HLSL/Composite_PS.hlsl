Texture2D g_BaseMap : register(t0); // 原纹理
Texture2D g_EdgeMap : register(t1); // 边缘纹理
SamplerState g_SamLinearWrap : register(s0); // 线性过滤+Wrap采样器
SamplerState g_SamPointClamp : register(s1); // 点过滤+Clamp采样器

float4 PS(float4 posH : SV_Position, float2 tex : TEXCOORD) : SV_Target
{
    float4 c = g_BaseMap.SampleLevel(g_SamPointClamp, tex, 0.0f);
    float4 e = g_EdgeMap.SampleLevel(g_SamPointClamp, tex, 0.0f);
    // 将原始图片与边缘图相乘
    return c * e;
}
