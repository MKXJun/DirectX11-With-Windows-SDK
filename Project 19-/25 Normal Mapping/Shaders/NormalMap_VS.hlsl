#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTangentTex VS(VertexPosNormalTangentTex vIn)
{
    VertexPosHWNormalTangentTex vOut;
    
    vector posW = mul(float4(vIn.posL, 1.0f), g_World);

    vOut.posW = posW.xyz;
    vOut.posH = mul(posW, g_ViewProj);
    vOut.normalW = mul(vIn.normalL, (float3x3) g_WorldInvTranspose);
    vOut.tangentW = float4(mul(vIn.tangentL.xyz, (float3x3) g_World), vIn.tangentL.w);
    vOut.tex = vIn.tex;
    return vOut;
}
