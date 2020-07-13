#include "Basic.hlsli"

[domain("tri")]
VertexOutNormalMap DS(PatchTess patchTess,
             float3 bary : SV_DomainLocation,
             const OutputPatch<HullOut, 3> tri)
{
    VertexOutNormalMap dOut;
    
    // 对面片属性进行插值以生成顶点
    dOut.PosW     = bary.x * tri[0].PosW     + bary.y * tri[1].PosW     + bary.z * tri[2].PosW;
    dOut.NormalW  = bary.x * tri[0].NormalW  + bary.y * tri[1].NormalW  + bary.z * tri[2].NormalW;
    dOut.TangentW = bary.x * tri[0].TangentW + bary.y * tri[1].TangentW + bary.z * tri[2].TangentW;
    dOut.Tex      = bary.x * tri[0].Tex      + bary.y * tri[1].Tex      + bary.z * tri[2].Tex;
    
    // 对插值后的法向量进行标准化
    dOut.NormalW = normalize(dOut.NormalW);
    
    //
    // 位移映射
    //
    
    // 基于摄像机到顶点的距离选取mipmap等级；特别地，对每个MipInterval单位选择下一个mipLevel
    // 然后将mipLevel限制在[0, 6]
    const float MipInterval = 20.0f;
    float mipLevel = clamp((distance(dOut.PosW, g_EyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);
    
    // 对高度图采样（存在法线贴图的alpha通道）
    float h = g_NormalMap.SampleLevel(g_Sam, dOut.Tex, mipLevel).a;
    
    // 沿着法向量进行偏移
    dOut.PosW += (g_HeightScale * (h - 1.0f)) * dOut.NormalW;
    
    // 生成投影纹理坐标
    dOut.ShadowPosH = mul(float4(dOut.PosW, 1.0f), g_ShadowTransform);
    
    // 投影到齐次裁减空间
    dOut.PosH = mul(float4(dOut.PosW, 1.0f), g_ViewProj);
    
    // 从NDC坐标[-1, 1]^2变换到纹理空间坐标[0, 1]^2
    // u = 0.5x + 0.5
    // v = -0.5y + 0.5
    // ((xw, yw, zw, w) + (w, w, 0, 0)) * (0.5, -0.5, 1, 1) = ((0.5x + 0.5)w, (-0.5y + 0.5)w, zw, w)
    //                                                      = (uw, vw, zw, w)
    //                                                      =>  (u, v, z, 1)
    dOut.SSAOPosH = (dOut.PosH + float4(dOut.PosH.ww, 0.0f, 0.0f)) * float4(0.5f, -0.5f, 1.0f, 1.0f);
    
    return dOut;
}