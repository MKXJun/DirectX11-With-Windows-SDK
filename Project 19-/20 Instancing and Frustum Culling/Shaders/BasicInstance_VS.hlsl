#include "Basic.hlsli"

// 顶点着色器
VertexPosHWNormalColorTex VS(InstancePosNormalTex vIn)
{
    VertexPosHWNormalColorTex vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), vIn.World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, g_ViewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) vIn.WorldInvTranspose);
    vOut.Color = vIn.Color;
    vOut.Tex = vIn.Tex;
    return vOut;
}
