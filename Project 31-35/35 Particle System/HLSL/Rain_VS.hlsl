#include "Rain.hlsli"

VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.Age;
    
    // 恒定加速度等式
    vOut.PosW = 0.5f * t * t * g_AccelW + t * vIn.InitialVelW + vIn.InitialPosW;
    
    vOut.Type = vIn.Type;
    
    return vOut;
}
