#include "Basic.hlsli"

// 顶点着色器(3D)
Vertex3DOut VS_3D(Vertex3DIn vIn)
{
    Vertex3DOut vOut;
    
    matrix viewProj = mul(gView, gProj);
    float4 posW = mul(float4(vIn.PosL, 1.0f), gWorld);
    // 若当前在绘制反射物体，先进行反射操作
    [flatten]
    if (gIsReflection)
    {
        posW = mul(posW, gReflection);
    }
    // 若当前在绘制阴影，先进行投影操作
    [flatten]
    if (gIsShadow)
    {
        posW = (gIsReflection ? mul(posW, gRefShadow) : mul(posW, gShadow));
    }

    vOut.PosH = mul(posW, viewProj);
    vOut.PosW = posW.xyz;
    vOut.NormalW = mul(vIn.NormalL, (float3x3) gWorldInvTranspose);
    vOut.Tex = vIn.Tex;
    return vOut;
}