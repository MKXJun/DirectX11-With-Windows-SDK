#include "BasicObject.fx"

// 顶点着色器(3D)
Vertex3DOut VS_3D(Vertex3DIn pIn)
{
    Vertex3DOut pOut;
    
    float4 posW = mul(float4(pIn.PosL, 1.0f), gWorld);
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

    pOut.PosH = mul(mul(posW, gView), gProj);
    pOut.PosW = mul(float4(pIn.PosL, 1.0f), gWorld).xyz;
    pOut.NormalW = mul(pIn.NormalL, (float3x3) gWorldInvTranspose);
    pOut.Tex = pIn.Tex;
    return pOut;
}