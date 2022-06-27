
#ifndef PBR_HLSL
#define PBR_HLSL

Texture2D g_AlbedoMap : register(t0);
Texture2D g_NormalMap : register(t1);
Texture2D g_MetallicMap : register(t2);
Texture2D g_RoughnessMap : register(t3);
Texture2D g_AoMap : register(t4);

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float4 tangentW)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    // 构建位于世界坐标系的切线空间
    float3 N = unitNormalW;
    float3 T = normalize(tangentW.xyz - dot(tangentW.xyz, N) * N); // 施密特正交化
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    // 将法线贴图采样的法向量从切线空间变换到世界坐标系
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float3 posW, float2 texcoord)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    // 使用偏导求面切线和副法线
    float3 Q1 = ddx_coarse(posW);
    float3 Q2 = ddy_coarse(posW);
    float2 st1 = ddx(texcoord);
    float2 st2 = ddy(texcoord);
    
    float3 N = unitNormalW;
    float3 T = normalize(Q1 * st2.y - Q2 * st2.x);
    float3 B = normalize(cross(N, T));

    float3x3 TBN = float3x3(T, B, N);

    // 将法线贴图采样的法向量从切线空间变换到世界坐标系
    float3 bumpedNormalW = mul(normalT, TBN);

    return bumpedNormalW;
}

// Shlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
float3 Fresnel_Schlick(float3 f0, float3 f90, float x)
{
    return f0 + (f90 - f0) * pow(saturate(1.0f - x), 5.0f);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float Diffuse_Burley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2.f * roughness * LdotH * LdotH;
    return Fresnel_Schlick(1.0f, fd90, NdotL).x * Fresnel_Schlick(1.0f, fd90, NdotV).x;
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(in float alpha, in float NdotH)
{
    static const float PI = 3.14159265f;
    static const float EPS = 1e-6f;
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1.0f)) + 1.0f;
    return alpha2 / max(EPS, PI * lower * lower);
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
// http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html
float G_Schlick_Smith_Hable(float alpha, float LdotH)
{
    return rcp(lerp(LdotH * LdotH, 1.0f, alpha * alpha * 0.25f));
}


float G_Schlick_GGX(float NdotV, float k)
{
    return NdotV / lerp(NdotV, 1.0f, k);
}

float G_Smith(float NdotV, float NdotL, float k)
{
    return G_Schlick_GGX(NdotV, k) * G_Schlick_GGX(NdotL, k);
}

// 基于微表面的BRDF
//
// roughness:       粗糙度
//
// specularColor:   F0反射项 - 非金属0.04，金属使用RGB。该模型遵循UE4
//
//      N - 表面法线
//      V - 摄像机视线方向
//      L - 光照方向
//      H - L和V的半程向量
float3 Specular_BRDF(float alpha,
                     float k,
                     float3 specularColor,
                     float NdotV,
                     float NdotL,
                     float HdotV,
                     float NdotH)
{
    // 微表面法线分布项
    // \alpha^2 / (\pi (NdotH)^2 (\alpha^2 - 1) + 1)^2 )
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // 镜面菲涅尔项
    // F_0 + (1 - F_0)(1 - HdotV)^5
    float3 specular_F = Fresnel_Schlick(specularColor, 1.0f, HdotV);

    // 镜面几何遮蔽项(可见性)
    // NdotV / ( NdotV(1 - k) + k ) * NdotL / ( NdotH(1 - k) + k )
    // k_direct = (\alpha + 1)^2 / 8
    // K_IBL = \alpha^2 / 2
    float specular_G = G_Smith(NdotV, NdotL, k);

    return specular_D * specular_F * specular_G;
}

// 对表面应用迪士尼风格的PBR:
//
// V, N:             摄像机视线方向和表面法线
//
// numLights:        Number of directional lights.
//
// lightColor[]:     Color and intensity of directional light.
//
// lightDirection[]: Light direction.
float3 AccumulateBRDF(
    float3 V, float3 N,
    int numLights, float3 lightColor[4], float3 lightDirection[4],
    float3 albedo, float roughness, float metallic, float ambientOcclusion)
{
    static const float PI = 3.14159265f;
    static const float EPS = 1e-6f;
    static const float specularCoefficient = 0.04f;
    
    const float NdotV = saturate(dot(N, V));
    
    // 漫反射项 - 只有非金属才有漫反射
    const float3 diffuse = lerp(albedo, 0.0f, metallic) / PI;
    // F0 - 对非金属使用固定反射值
    const float3 F0 = lerp(specularCoefficient, albedo, metallic);

    float alpha = roughness * roughness;
    // k_direct
    float k = (alpha + 1) * (alpha + 1) / 8;
    
    float3 acc_color = 0;
    for (int i = 0; i < numLights; i++)
    {
        // 指向光照的向量
        const float3 L = normalize(-lightDirection[i]);

        // 半程向量
        const float3 H = normalize(L + V);

        const float NdotL = saturate(dot(N, L));
        const float NdotH = saturate(dot(N, H));
        const float HdotV = saturate(dot(H, V));
        // 漫反射 & 镜面高光项
        
        // 微表面法线分布项
        // \alpha^2 / (\pi (NdotH)^2 (\alpha^2 - 1) + 1)^2 )
        float specular_D = Specular_D_GGX(alpha, NdotH);

        // 镜面菲涅尔项
        // F_0 + (1 - F_0)(1 - HdotV)^5
        float3 specular_F = Fresnel_Schlick(F0, 1.0f, HdotV);

        // 镜面几何遮蔽项(可见性)
        // NdotV / ( NdotV(1 - k) + k ) * NdotL / ( NdotH(1 - k) + k )
        // k_direct = (\alpha + 1)^2 / 8
        // K_IBL = \alpha^2 / 2
        float specular_G = G_Smith(NdotV, NdotL, k);
        
        float3 specular = specular_D * specular_F * specular_G;
        specular *= rcp(4.0f * NdotV * NdotL + 0.0001f);

        // specularF作为kS
        //   lerp(diff, specular, specular_F)
        // = (1 - kS) diff + kS * specular
        acc_color += NdotL * lightColor[i] * lerp(diffuse, specular, specular_F);
    }

    // 环境光照(后续会使用env lighting替换ambient lighting
    float3 ambient = 0.03f * albedo * ambientOcclusion;
    acc_color += ambient;

    return acc_color;
}

#endif // PBR_HLSL
