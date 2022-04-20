#include "Basic.hlsli"

// 顶点着色器
VertexOutNormalMap VS(InstancePosNormalTangentTex vIn)
{
    VertexOutNormalMap vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), vIn.World);

    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, g_ViewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) vIn.WorldInvTranspose);
    vOut.TangentW = mul(vIn.TangentL, vIn.World);
    vOut.Tex = vIn.Tex;
    vOut.ShadowPosH = mul(posW, g_ShadowTransform);
    return vOut;
}
