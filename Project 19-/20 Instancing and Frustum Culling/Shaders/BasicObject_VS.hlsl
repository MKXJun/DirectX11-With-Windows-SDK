#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalColorTex VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalColorTex vOut;
    
    vector posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posW = posW.xyz;
    vOut.posH = mul(posW, g_ViewProj);
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.color = g_ConstantDiffuseColor;
    vOut.tex = vIn.tex;
    return vOut;
}
