
#ifndef RENDERING_HLSL
#define RENDERING_HLSL

#include "FullScreenTriangle.hlsl"
#include "ConstantBuffers.hlsl"

//--------------------------------------------------------------------------------------
// 几何阶段 
//--------------------------------------------------------------------------------------
Texture2D g_TextureDiffuse : register(t0);
SamplerState g_SamplerDiffuse : register(s0);

struct VertexPosNormalTex
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VertexOut
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 texCoord : TEXCOORD0;
    float4 shadowPosH : TEXCOORD1;
};

VertexOut GeometryVS(VertexPosNormalTex input)
{
    VertexOut output;

    output.posH = mul(float4(input.posL, 1.0f), g_WorldViewProj);
    output.posW = mul(float4(input.posL, 1.0f), g_World).xyz;
    output.normalW = mul(float4(input.normalL, 0.0f), g_WorldInvTranspose).xyz;
    output.texCoord = input.texCoord;
    output.shadowPosH = mul(float4(output.posW, 1.0f), g_ShadowTransform);
    
    return output;
}

struct SurfaceData
{
    float3 posW;
    float3 normalW;
    float4 albedo;
    float3 specularAmount;
    float specularPower;
};

SurfaceData ComputeSurfaceDataFromGeometry(VertexOut input)
{
    SurfaceData surface;
    surface.posW = input.posW;
    surface.normalW = input.normalW;
    input.texCoord.y = 1.0f - input.texCoord.y;
    surface.albedo = g_TextureDiffuse.Sample(g_SamplerDiffuse, input.texCoord);
    
    // 将空漫反射纹理映射为白色
    uint2 textureDim;
    g_TextureDiffuse.GetDimensions(textureDim.x, textureDim.y);
    surface.albedo = (textureDim.x == 0U ? float4(1.0f, 1.0f, 1.0f, 1.0f) : surface.albedo);
    
    surface.specularAmount = g_Kspecular.rgb;
    surface.specularPower = 32.0f;
    
    return surface;
}

//--------------------------------------------------------------------------------------
// 光照阶段 
//--------------------------------------------------------------------------------------

float3 BlinnPhong(SurfaceData surface)
{
    float3 ambient = 0.05f * surface.albedo.rgb;
    
    float3 lightDir = normalize(g_LightPosW);
    float3 normal = normalize(surface.normalW);
    float diff = max(dot(lightDir, normal), 0.0f);
    float3 light_atten_coff = g_LightIntensity / pow(length(g_LightPosW - surface.posW), 2.0f);
    float3 diffuse = diff * light_atten_coff * surface.albedo.rgb;
    
    float3 viewDir = normalize(g_CameraPosW - surface.posW);
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfDir, normal), 0.0f), surface.specularPower);
    float3 specular = g_Kspecular.rgb * light_atten_coff * spec;
    
    float3 radiance = ambient + diffuse + specular;
    return radiance;
}

#endif // RENDERING_HLSL
