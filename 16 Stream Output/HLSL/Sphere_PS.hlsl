#include "Basic.hlsli"

float4 PS(VertexPosHWNormalColor pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);

    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // 只计算方向光
    ComputeDirectionalLight(g_Material, g_DirLight[0], pIn.NormalW, toEyeW, ambient, diffuse, spec);

    return pIn.Color * (ambient + diffuse) + spec;
}
