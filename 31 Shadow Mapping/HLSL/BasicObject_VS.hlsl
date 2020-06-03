#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalTexShadowPosH VS(VertexPosNormalTex vIn)
{
    VertexPosHWNormalTexShadowPosH vOut;
    
    matrix viewProj = mul(g_View, g_Proj);
    vector posW = mul(float4(vIn.PosL, 1.0f), g_World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, viewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) g_WorldInvTranspose);
    vOut.Tex = vIn.Tex;
    vOut.ShadowPosH = mul(posW, g_ShadowTransform);
    return vOut;
}
