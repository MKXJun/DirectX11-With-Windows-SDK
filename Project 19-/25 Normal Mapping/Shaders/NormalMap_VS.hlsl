#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTangentTex VS(VertexPosNormalTangentTex vIn)
{
    VertexPosHWNormalTangentTex vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), g_World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, g_ViewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.TangentW = float4(mul(vIn.TangentL.xyz, (float3x3) g_WorldInvTranspose), vIn.TangentL.w);
    vOut.Tex = vIn.Tex;
    return vOut;
}