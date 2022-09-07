
#ifndef PBR_HLSL
#define PBR_HLSL

#include "Constants.hlsl"
#include "ConstantBuffers.hlsl"


struct SurfaceData
{
    float4 albedo;
    float3 posV;
    float roughness;
    float3 normalV;
    float metallic;
    float3 F0;
    float k_direct;
    float ambientOcclusion;
};

struct PointLight
{
    float3 posV;
    float range;
    float3 color;
    float intensity;
};

Texture2D g_AlbedoMap : register(t0);
Texture2D g_NormalMap : register(t1);

Texture2D g_RoughnessMetallicMap : register(t2); // G-roughness, B-metallic
Texture2D g_RoughnessMap : register(t3);
Texture2D g_MetallicMap : register(t4);

StructuredBuffer<PointLight> g_Light : register(t5);

float3 TransformNormalSample(float3 normalMapSample, float3 unitNormal, float4 tangent)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;

    // 构建位于世界坐标系的切线空间
    float3 N = unitNormal;
    float3 T = normalize(tangent.xyz - dot(tangent.xyz, N) * N); // 施密特正交化
    float3 B = cross(N, T);

    float3x3 TBN = float3x3(T, B, N);

    // 将法线贴图采样的法向量从切线空间变换到世界坐标系
    float3 bumpedNormal = mul(normalT, TBN);

    return normalize(bumpedNormal);
}

float3 TransformNormalSample(float3 normalMapSample, float3 unitNormal, float3 pos, float2 texcoord)
{
    // 将读取到法向量中的每个分量从[0, 1]还原到[-1, 1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;
    
    // 使用偏导求面切线和副法线
    float3 Q1 = ddx_coarse(pos);
    float3 Q2 = ddy_coarse(pos);
    float2 st1 = ddx_coarse(texcoord);
    float2 st2 = ddy_coarse(texcoord);
    
    float3 N = unitNormal;
    float3 T = normalize(Q1 * st2.y - Q2 * st2.x);
    float3 B = normalize(cross(N, T));

    float3x3 TBN = float3x3(T, B, N);

    // 将法线贴图采样的法向量从切线空间变换到世界坐标系
    float3 bumpedNormal = mul(normalT, TBN);

    return normalize(bumpedNormal);
}

// Schlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
float3 Fresnel_Schlick(float3 f0, float3 f90, float x)
{
    return f0 + (f90 - f0) * pow(saturate(1.0f - x), 5.0f);
}

float Fresnel_Schlick(float f0, float f90, float x)
{
    return f0 + (f90 - f0) * pow(saturate(1.0f - x), 5.0f);
}

float3 Fresnel_Schlick_Roughness(float cosTheta, float3 f0, float roughness)
{
    return f0 + (max(1.0f - roughness, f0) - f0) * pow(saturate(1.0f - cosTheta), 5.0f);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float Diffuse_Burley(float NdotL, float NdotV, float LdotH, float roughness)
{
    const float fd90 = 0.5f + 2.0f * roughness * LdotH * LdotH;
    return Fresnel_Schlick(1.0f, fd90, NdotL) * Fresnel_Schlick(1.0f, fd90, NdotV);
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(float alpha, float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1.0f)) + 1.0f;
    return alpha2 / max(1e-6f, X_PI * lower * lower);
}

float G_Schlick_GGX(float NdotV, float k)
{
    return NdotV / lerp(NdotV, 1.0f, k);
}

// Smith specular G (geometry obstruction)
float G_Smith(float NdotV, float NdotL, float k)
{
    return G_Schlick_GGX(NdotV, k) * G_Schlick_GGX(NdotL, k);
}

// Point light attenuation for deferred rendering
float CalcAttenuation(float3 lightVec, float range)
{
    float lengthSq = dot(lightVec, lightVec);
    float attenu = lengthSq / (range * range);
    attenu = saturate(1.0f - attenu * attenu);
    attenu = attenu * attenu / lengthSq;
    return attenu;
}

float3 CalcDirectionalLight(
    SurfaceData surfaceData,
    float3 lightDir,
    float3 lightColor,
    float lightIntensity)
{
    float3 radiance = lightColor * lightIntensity;
    const float3 V = normalize(-surfaceData.posV);
    const float3 L = normalize(-lightDir);
    // 半程向量
    const float3 H = normalize(L + V);

    const float NdotV = saturate(dot(surfaceData.normalV, V));
    const float NdotL = saturate(dot(surfaceData.normalV, L));
    const float NdotH = saturate(dot(surfaceData.normalV, H));
    const float HdotV = saturate(dot(H, V));
     
    float alpha = surfaceData.roughness * surfaceData.roughness;
    // 微表面法线分布项
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // 镜面菲涅尔项
    float3 specular_F = Fresnel_Schlick(surfaceData.F0, 1.0f, HdotV);

    // 镜面几何遮蔽项(可见性)
    float specular_G = G_Smith(NdotV, NdotL, surfaceData.k_direct);
        
    // 镜面高光项
    float3 specular = specular_D * specular_F * specular_G;
    specular *= rcp(4.0f * NdotV * NdotL + 0.0001f);
    
    float3 kS = specular_F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - surfaceData.metallic;
    
    float3 litColor = (kD * surfaceData.albedo.rgb + specular) * radiance * NdotL;
    return litColor;
}

float3 CalcPointLight(
    SurfaceData surfaceData,
    PointLight light)
{
    const float3 V = normalize(-surfaceData.posV);

    float alpha = surfaceData.roughness * surfaceData.roughness;
    
    // 指向光照的向量
    float3 L = light.posV - surfaceData.posV;
    
    // 衰减
    float attenu = CalcAttenuation(L, light.range);
    float3 radiance = light.color * light.intensity * attenu;
    
    L = normalize(L);
    
    // 半程向量
    const float3 H = normalize(L + V);

    const float NdotV = saturate(dot(surfaceData.normalV, V));
    const float NdotL = saturate(dot(surfaceData.normalV, L));
    const float NdotH = saturate(dot(surfaceData.normalV, H));
    const float HdotV = saturate(dot(H, V));
        
    // 微表面法线分布项
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // 镜面菲涅尔项
    float3 specular_F = Fresnel_Schlick(surfaceData.F0, 1.0f, HdotV);

    // 镜面几何遮蔽项(可见性)
    float specular_G = G_Smith(NdotV, NdotL, surfaceData.k_direct);
        
    // 镜面高光项
    float3 specular = specular_D * specular_F * specular_G;
    specular *= rcp(4.0f * NdotV * NdotL + 0.0001f);
    
    float3 kS = specular_F;
    float3 kD = 1.0f - kS;
    kD *= 1.0f - surfaceData.metallic;
    
    float3 litColor = (kD * surfaceData.albedo.rgb + specular) * radiance * NdotL;
    return litColor;
}



#endif // PBR_HLSL
