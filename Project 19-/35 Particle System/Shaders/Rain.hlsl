#ifndef RAIN_HLSL
#define RAIN_HLSL

#include "Particle.hlsl"

// 绘制输出
struct VertexOut
{
    float3 posW : POSITION;
    uint Type : TYPE;
};

struct GeoOut
{
    float4 posH : SV_position;
    float2 tex : TEXCOORD;
};

VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.age;
    
    // 恒定加速度等式
    vOut.posW = 0.5f * t * t * g_AccelW + t * vIn.initialVelW + vIn.initialPosW;
    
    vOut.Type = vIn.type;
    
    return vOut;
}

[maxvertexcount(6)]
void GS(point VertexOut gIn[1], inout LineStream<GeoOut> output)
{
    // 不要绘制用于产生粒子的顶点
    if (gIn[0].Type != PT_EMITTER)
    {
        // 使线段沿着一个加速度方向倾斜
        float3 p0 = gIn[0].posW;
        float3 p1 = gIn[0].posW + 0.07f * g_AccelW;
        
        GeoOut v0;
        v0.posH = mul(float4(p0, 1.0f), g_ViewProj);
        v0.tex = float2(0.0f, 0.0f);
        output.Append(v0);
        
        GeoOut v1;
        v1.posH = mul(float4(p1, 1.0f), g_ViewProj);
        v1.tex = float2(0.0f, 0.0f);
        output.Append(v1);
    }
}

float4 PS(GeoOut pIn) : SV_Target
{
    return g_TextureInput.Sample(g_SamLinear, pIn.tex);
}

VertexParticle SO_VS(VertexParticle vIn)
{
    return vIn;
}

[maxvertexcount(6)]
void SO_GS(point VertexParticle gIn[1], inout PointStream<VertexParticle> output)
{
    gIn[0].age += g_TimeStep;
    
    if (gIn[0].type == PT_EMITTER)
    {
        // 是否到时间发射新的粒子
        if (gIn[0].age > g_EmitInterval)
        {
            [unroll]
            for (int i = 0; i < 5; ++i)
            {
                // 在摄像机上方的区域让雨滴降落
                float3 vRandom = 30.0f * RandVec3((float) i / 5.0f);
                vRandom.y = 20.0f;
                
                VertexParticle p;
                p.initialPosW = g_EmitPosW.xyz + vRandom;
                p.initialVelW = float3(0.0f, 0.0f, 0.0f);
                p.sizeW = float2(1.0f, 1.0f);
                p.age = 0.0f;
                p.type = PT_PARTICLE;
                
                output.Append(p);
            }
            
            // 重置时间准备下一次发射
            gIn[0].age = 0.0f;
        }
        
        // 总是保留发射器
        output.Append(gIn[0]);
    }
    else
    {
        // 用于限制粒子数目产生的特定条件，对于不同的粒子系统限制也有所变化
        if (gIn[0].age <= g_AliveTime)
            output.Append(gIn[0]);
    }
}

#endif
