#ifndef PARTICLE_HLSL
#define PARTICLE_HLSL

cbuffer CBChangesEveryFrame : register(b0)
{
    matrix g_ViewProj;
    
    float3 g_EyePosW;
    float g_GameTime;
    
    float g_TimeStep;
    float3 g_EmitDirW;
    
    float3 g_EmitPosW;
    float g_EmitInterval;
    
    float g_AliveTime;
}

cbuffer CBFixed : register(b1)
{
    // 用于加速粒子运动的加速度
    float3 g_AccelW;
}

// 用于贴图到粒子上的纹理
Texture2D g_TextureInput : register(t0);

// 用于在着色器中生成随机数的纹理
Texture1D g_TextureRandom : register(t1);

SamplerState g_SamLinear : register(s0);


float3 RandUnitVec3(float offset)
{
    // 使用游戏时间加上偏移值来采样随机纹理
    float u = (g_GameTime + offset);
    
    // 采样值在[-1,1]
    float3 v = g_TextureRandom.SampleLevel(g_SamLinear, u, 0).xyz;
    
    // 投影到单位球
    return normalize(v);
}

float3 RandVec3(float offset)
{
    // 使用游戏时间加上偏移值来采样随机纹理
    float u = (g_GameTime + offset);
    
    // 采样值在[-1,1]
    float3 v = g_TextureRandom.SampleLevel(g_SamLinear, u, 0).xyz;
    
    return v;
}

#define PT_EMITTER 0
#define PT_PARTICLE 1

struct VertexParticle
{
    float3 initialPosW : POSITION;
    float3 initialVelW : VELOCITY;
    float2 sizeW : SIZE;
    float age : AGE;
    uint type : TYPE;
};

#endif