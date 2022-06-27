#include "Basic.hlsli"
#include "OIT.hlsli"

// 静态链表创建
// 强制开启早期深度/模板测试，避免产生不符合深度的像素片元的节点
[earlydepthstencil]
void PS(VertexPosHWNormalTex pIn)
{
    float4 texColor = g_DiffuseMap.Sample(g_SamLinearWrap, pIn.tex);
    // 提前进行Alpha裁剪，对不符合要求的像素可以避免后续运算
    clip(texColor.a - 0.1f);
    
    // 标准化法向量
    pIn.normalW = normalize(pIn.normalW);

    // 求出顶点指向眼睛的向量，以及顶点与眼睛的距离
    float3 toEyeW = normalize(g_EyePosW - pIn.posW);
    float distToEye = distance(g_EyePosW, pIn.posW);

    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int i;

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeDirectionalLight(g_Material, g_DirLight[i], pIn.normalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputePointLight(g_Material, g_PointLight[i], pIn.posW, pIn.normalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeSpotLight(g_Material, g_SpotLight[i], pIn.posW, pIn.normalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
  
    float4 litColor = texColor * (ambient + diffuse) + spec;

    // 雾效部分
    [flatten]
    if (g_FogEnabled)
    {
        // 限定在0.0f到1.0f范围
        float fogLerp = saturate((distToEye - g_FogStart) / g_FogRange);
        // 根据雾色和光照颜色进行线性插值
        litColor = lerp(litColor, g_FogColor, fogLerp);
    }
    
    litColor.a = texColor.a * g_Material.diffuse.a;

    litColor = saturate(litColor);
    
    // 取得当前像素数目并自递增计数器
    uint pixelCount = g_FLBufferRW.IncrementCounter();
    
    // 在StartOffsetBuffer实现值交换
    uint2 vPos = (uint2) pIn.posH.xy;  
    uint startOffsetAddress = 4 * (g_FrameWidth * vPos.y + vPos.x);
    uint oldStartOffset;
    g_StartOffsetBufferRW.InterlockedExchange(
        startOffsetAddress, pixelCount, oldStartOffset);
    
    // 向片元/链接缓冲区添加新的节点
    FLStaticNode node;
    // 压缩颜色为R8G8B8A8
    node.data.color = PackColorFromFloat4(litColor);
    node.data.depth = pIn.posH.z;
    node.next = oldStartOffset;
    
    g_FLBufferRW[pixelCount] = node;
}
