#include "Basic.hlsli"
#include "OIT.hlsli"

RWStructuredBuffer<FLStaticNode> g_FLBuffer : register(u1);
RWByteAddressBuffer g_StartOffsetBuffer : register(u2);

// 静态链表创建
// 提前开启深度/模板测试，避免产生不符合深度的像素片元的节点
[earlydepthstencil]
void PS(VertexPosHWNormalTex pIn)
{
    // 若不使用纹理，则使用默认白色
    float4 texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (g_TextureUsed)
    {
        texColor = g_DiffuseMap.Sample(g_SamLinearWrap, pIn.Tex);
        // 提前进行裁剪，对不符合要求的像素可以避免后续运算
        clip(texColor.a - 0.1f);
    }
    
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 求出顶点指向眼睛的向量，以及顶点与眼睛的距离
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);
    float distToEye = distance(g_EyePosW, pIn.PosW);

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
        ComputeDirectionalLight(g_Material, g_DirLight[i], pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputePointLight(g_Material, g_PointLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeSpotLight(g_Material, g_SpotLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
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
    
    litColor.a = texColor.a * g_Material.Diffuse.a;

    litColor = saturate(litColor);
    
    // 取得当前像素数目并自递增计数器
    uint pixelCount = g_FLBuffer.IncrementCounter();
    
    // 在StartOffsetBuffer实现值交换
    uint2 vPos = (uint2) pIn.PosH.xy;  
    uint startOffsetAddress = 4 * (g_FrameWidth * vPos.y + vPos.x);
    uint oldStartOffset;
    g_StartOffsetBuffer.InterlockedExchange(
        startOffsetAddress, pixelCount, oldStartOffset);
    
    // 向片元/链接缓冲区添加新的节点
    FLStaticNode node;
    // 压缩颜色为R8G8B8A8
    node.Data.Color = PackColorFromFloat4(litColor);
    node.Data.Depth = pIn.PosH.z;
    node.Next = oldStartOffset;
    
    g_FLBuffer[pixelCount] = node;
}
