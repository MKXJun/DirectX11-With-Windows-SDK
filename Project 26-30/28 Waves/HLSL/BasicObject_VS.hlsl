#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    
    // 绘制水波时用到
    if (g_WavesEnabled)
    {
        // 使用映射到[0,1]x[0,1]区间的纹理坐标进行采样
        vIn.PosL.y += g_DisplacementMap.SampleLevel(g_SamLinearWrap, vIn.Tex, 0.0f).r;
        // 使用有限差分法估算法向量
        float du = g_DisplacementMapTexelSize.x;
        float dv = g_DisplacementMapTexelSize.y;
        float left = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.Tex - float2(du, 0.0f), 0.0f).r;
        float right = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.Tex + float2(du, 0.0f), 0.0f).r;
        float top = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.Tex - float2(0.0f, dv), 0.0f).r;
        float bottom = g_DisplacementMap.SampleLevel(g_SamPointClamp, vIn.Tex + float2(0.0f, dv), 0.0f).r;
        vIn.NormalL = normalize(float3(-right + left, 2.0f * g_GridSpatialStep, bottom - top));
    }
    
    matrix viewProj = mul(g_View, g_Proj);
    vector posW = mul(float4(vIn.PosL, 1.0f), g_World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, viewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.Tex = mul(float4(vIn.Tex, 0.0f, 1.0f), g_TexTransform).xy;
    return vOut;
}