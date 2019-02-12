#include "Basic.hlsli"

// 像素着色器(3D)
float4 PS_3D(VertexPosHWNormalTex pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(gEyePosW - pIn.PosW);

    // 初始化为0 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 A = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 D = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 S = float4(0.0f, 0.0f, 0.0f, 0.0f);
    int i;
	// 强制展开循环以减少指令数
	[unroll]
    for (i = 0; i < gNumDirLight; ++i)
    {
        ComputeDirectionalLight(gMaterial, gDirLight[i], pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    
	[unroll]
    for (i = 0; i < gNumPointLight; ++i)
    {
        ComputePointLight(gMaterial, gPointLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    
	[unroll]
    for (i = 0; i < gNumSpotLight; ++i)
    {
        ComputeSpotLight(gMaterial, gSpotLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
    

    float4 texColor = gTex.Sample(gSamLinear, pIn.Tex);
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * gMaterial.Diffuse.a;
	
    return litColor;
}