#include "Shadow.hlsli"

[domain("tri")]
VertexPosHTex DS(PatchTess patchTess,
             float3 bary : SV_DomainLocation,
             const OutputPatch<HullOut, 3> tri)
{
    VertexPosHTex dOut;
    
    // 对面片属性进行插值以生成顶点
    float3 posW    = bary.x * tri[0].PosW     + bary.y * tri[1].PosW     + bary.z * tri[2].PosW;
    float3 normalW = bary.x * tri[0].NormalW  + bary.y * tri[1].NormalW  + bary.z * tri[2].NormalW;
    dOut.Tex       = bary.x * tri[0].Tex      + bary.y * tri[1].Tex      + bary.z * tri[2].Tex;
    
    // 对插值后的法向量进行标准化
    normalW = normalize(normalW);
    
    //
    // 位移映射
    //
    
    // 基于摄像机到顶点的距离选取mipmap等级；特别地，对每个MipInterval单位选择下一个mipLevel
    // 然后将mipLevel限制在[0, 6]
    const float MipInterval = 20.0f;
    float mipLevel = clamp((distance(posW, g_EyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
    
    // 对高度图采样（存在法线贴图的alpha通道）
    float h = g_NormalMap.SampleLevel(g_Sam, dOut.Tex, mipLevel).a;
    
    // 沿着法向量进行偏移
    posW += (g_HeightScale * (h - 1.0f)) * normalW;
    
    // 投影到齐次裁减空间
    dOut.PosH = mul(float4(posW, 1.0f), g_ViewProj);
    
    return dOut;
}