#include "Basic.hlsli"

// 顶点着色器
VertexOutBasic VS(InstancePosNormalTex vIn)
{
    VertexOutBasic vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), vIn.World);
    matrix viewProj = mul(g_View, g_Proj);
    
    vOut.PosW = posW.xyz;
    vOut.PosH = mul(posW, viewProj);
    vOut.NormalW = mul(vIn.NormalL, (float3x3) vIn.WorldInvTranspose);
    vOut.Tex = vIn.Tex;
    vOut.ShadowPosH = mul(posW, g_ShadowTransform);
    
    // 从NDC坐标[-1, 1]^2变换到纹理空间坐标[0, 1]^2
    // u = 0.5x + 0.5
    // v = -0.5y + 0.5
    // ((xw, yw, zw, w) + (w, w, 0, 0)) * (0.5, -0.5, 1, 1) = ((0.5x + 0.5)w, (-0.5y + 0.5)w, zw, w)
    //                                                      = (uw, vw, zw, w)
    //                                                      =>  (u, v, z, 1)
    vOut.SSAOPosH = (vOut.PosH + float4(vOut.PosH.ww, 0.0f, 0.0f)) * float4(0.5f, -0.5f, 1.0f, 1.0f);
    return vOut;
}
