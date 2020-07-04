#include "SSAO.hlsli"

float NdcDepthToViewDepth(float z_ndc)
{
    // z_ndc = A + B / viewZ, g_Proj(2, 2) = A, g_Proj(3, 2) = B
    float viewZ = g_Proj[3][2] / (z_ndc - g_Proj[2][2]);
    return viewZ;
}

// 双边滤波
float4 PS(VertexPosHTex pIn, uniform bool horizontalBlur) : SV_Target
{
    // 解包到浮点数组
    float blurWeights[12] = (float[12]) g_BlurWeights;
    
    float2 texOffset;
    if (horizontalBlur)
    {
        texOffset = float2(1.0f / g_InputImage.Length.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0f, 1.0f / g_InputImage.Length.y);
    }
    
    // 总是把中心值加进去计算
    float4 color = blurWeights[g_BlurRadius] * g_InputImage.SampleLevel(g_SamBlur, pIn.Tex, 0.0f);
    float totalWeight = blurWeights[g_BlurRadius];
    
    float4 centerNormalDepth = g_NormalDepthMap.SampleLevel(g_SamBlur, pIn.Tex, 0.0f);
    // 分拆出法向量，并计算在观察空间的深度
    float3 centerNormal = centerNormalDepth.xyz;
    float centerDepth = NdcDepthToViewDepth(centerNormalDepth.w);
    
    for (float i = -g_BlurRadius; i <= g_BlurRadius; ++i)
    {
        // 我们已经将中心值加进去了
        if (i == 0)
            continue;
        
        float2 tex = pIn.Tex + i * texOffset;
        
        float4 neighborNormalDepth = g_NormalDepthMap.SampleLevel(g_SamBlur, tex, 0.0f);
        // 分拆出法向量，并计算在观察空间的深度
        float3 neighborNormal = neighborNormalDepth.xyz;
        float neighborDepth = NdcDepthToViewDepth(neighborNormalDepth.w);
        
        //
        // 如果中心值和相邻值的深度或法向量相差太大，我们就认为当前采样点处于边缘区域，
        // 因此不考虑加入当前相邻值
        //
        
        if (dot(neighborNormal, centerNormal) >= 0.8f && abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = blurWeights[i + g_BlurRadius];
            
            // 将相邻像素加入进行模糊
            color += weight * g_InputImage.SampleLevel(g_SamBlur, tex, 0.0f);
            totalWeight += weight;
        }
        
    }

    // 通过让总权值变为1来补偿丢弃的采样像素
    return color / totalWeight;
}

