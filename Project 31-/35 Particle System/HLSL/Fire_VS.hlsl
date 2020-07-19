#include "Fire.hlsli"

VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.Age;
    
    // 恒定加速度等式
    vOut.PosW = 0.5f * t * t * g_AccelW + t * vIn.InitialVelW + vIn.InitialPosW;
    
    // 颜色随着时间褪去
    float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0f);
    vOut.Color = float4(1.0f, 1.0f, 1.0f, opacity);
    
    vOut.SizeW = vIn.SizeW;
    vOut.Type = vIn.Type;
    
    return vOut;
}
