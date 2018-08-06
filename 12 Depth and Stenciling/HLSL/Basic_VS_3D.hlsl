#include "Basic.fx"

// 顶点着色器(3D)
VertexOut VS_3D(VertexIn pIn)
{
    VertexOut pOut;
    
    float4 posH = mul(float4(pIn.Pos, 1.0f), gWorld);
    // 若当前在绘制反射物体，先进行反射操作
    [flatten]
    if (gIsReflection)
    {
        posH = mul(posH, gReflection);
    }
    pOut.PosH = mul(mul(posH, gView), gProj);
    pOut.PosW = mul(float4(pIn.Pos, 1.0f), gWorld).xyz;
    pOut.NormalW = mul(pIn.Normal, (float3x3) gWorldInvTranspose);
    pOut.Tex = mul(float4(pIn.Tex, 0.0f, 1.0f), gTexTransform).xy;
    return pOut;
}
