#include "Basic.hlsli"

// 像素着色器(3D)
float4 PS_3D(VertexPosHWNormalTex pIn) : SV_Target
{
	// 提前进行裁剪，对不符合要求的像素可以避免后续运算
    float4 texColor = gTex.Sample(gSam, pIn.Tex);
    clip(texColor.a - 0.1f);

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
        DirectionalLight dirLight = gDirLight[i];
        [flatten]
        if (gIsReflection)
        {
            dirLight.Direction = mul(dirLight.Direction, (float3x3) (gReflection));
        }
        ComputeDirectionalLight(gMaterial, gDirLight[i], pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    

    
    // 若当前在绘制反射物体，需要对光照进行反射矩阵变换
    PointLight pointLight;
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        pointLight = gPointLight[i];
        [flatten]
        if (gIsReflection)
        {
            pointLight.Position = mul(float4(pointLight.Position, 1.0f), gReflection).xyz;
        }
        ComputePointLight(gMaterial, pointLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    
	
    SpotLight spotLight;
    // 若当前在绘制反射物体，需要对光照进行反射矩阵变换
    [unroll]
    for (i = 0; i < 5; ++i)
    {
        spotLight = gSpotLight[i];
        [flatten]
        if (gIsReflection)
        {
            spotLight.Position = (float3) mul(float4(spotLight.Position, 1.0f), gReflection);
            spotLight.Direction = mul(spotLight.Direction, (float3x3) gReflection);
        }
        ComputeSpotLight(gMaterial, spotLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
        ambient += A;
        diffuse += D;
        spec += S;
    }
        
    

	
    float4 litColor = texColor * (ambient + diffuse) + spec;
    litColor.a = texColor.a * gMaterial.Diffuse.a;
    return litColor;
}
