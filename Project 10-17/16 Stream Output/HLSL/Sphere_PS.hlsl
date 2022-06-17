#include "Basic.hlsli"

float4 PS(VertexPosHWNormalColor pIn) : SV_Target
{
    // 标准化法向量
    pIn.normalW = normalize(pIn.normalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(g_EyePosW - pIn.posW);

    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 只计算方向光
    ComputeDirectionalLight(g_Material, g_DirLight[0], pIn.normalW, toEyeW, ambient, diffuse, spec);

    return pIn.color * (ambient + diffuse) + spec;
}
