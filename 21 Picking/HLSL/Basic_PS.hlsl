#include "Basic.fx"

// 像素着色器(3D)
float4 PS(VertexPosHWNormalTex pIn) : SV_Target
{
	// 提前进行裁剪，对不符合要求的像素可以避免后续运算

    // 若不使用纹理，则使用默认白色
    float4 texColorA = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 texColorD = float4(1.0f, 1.0f, 1.0f, 1.0f);

    if (gTextureUsed)
    {
        texColorA = texA.Sample(sam, pIn.Tex);
        texColorD = texD.Sample(sam, pIn.Tex);
        clip(texColorA.a - 0.1f);
        clip(texColorD.a - 0.1f);
    }
    
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

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeDirectionalLight(gMaterial, gDirLight[i], pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputePointLight(gMaterial, gPointLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }

    [unroll]
    for (i = 0; i < 5; ++i)
    {
        ComputeSpotLight(gMaterial, gSpotLight[i], pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
  
    
    float4 litColor = texColorA * ambient + texColorD * diffuse + spec;
    litColor.a = texColorD.a * gMaterial.Diffuse.a;
    return litColor;
}
