#include "OIT.hlsli"

// 顶点着色器
float4 VS(float3 vPos : POSITION) : SV_Position
{
    return float4(vPos, 1.0f);
}