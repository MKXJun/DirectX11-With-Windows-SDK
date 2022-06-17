#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTex VS(InstancePosNormalTex vIn)
{
    VertexPosHWNormalTex vOut;
    
    vector posW = mul(float4(vIn.posL, 1.0f), vIn.world);

    vOut.posW = posW.xyz;
    vOut.posH = mul(posW, g_ViewProj);
    vOut.normalW = mul(vIn.normalL, (float3x3) vIn.worldInvTranspose);
    vOut.tex = vIn.tex;
    return vOut;
}
