#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    
    // 绘制水波时用到
    if (g_WavesEnabled)
    {
        // 使用映射到[0,1]x[0,1]区间的纹理坐标进行采样
        vIn.posL.y += g_DisplacementMap.SampleLevel(g_SamLinearWrap, vIn.tex, 0.0f).r;
        // 使用有限差分法估算法向量
        float left = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(-1, 0)).r;
        float right = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(1, 0)).r;
        float top = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(0, -1)).r;
        float bottom = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.tex, 0.0f, int2(0, 1)).r;
        vIn.normalL = normalize(float3(-right + left, 2.0f * g_GridSpatialStep, bottom - top));
    }
    
    vector posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posW = posW.xyz;
    vOut.posH = mul(posW, g_ViewProj);
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.tex = mul(float4(vIn.tex, 0.0f, 1.0f), g_TexTransform).xy;
    return vOut;
}