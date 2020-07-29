#include "SSAO.hlsli"

// 生成观察空间的法向量和深度值的RTT的像素着色器
float4 PS(VertexPosHVNormalVTex pIn, uniform bool alphaClip) : SV_TARGET
{
    // 将法向量给标准化
    pIn.NormalV = normalize(pIn.NormalV);
    
    if (alphaClip)
    {
        float4 g_TexColor = g_DiffuseMap.Sample(g_SamLinearWrap, pIn.Tex);
        
        clip(g_TexColor.a - 0.1f);
    }
    
    // 返回观察空间的法向量和深度值
    return float4(pIn.NormalV, pIn.PosV.z);
}
