#include "SSAO.hlsli"

// 生成观察空间的法向量和深度值的RTT的顶点着色器
VertexPosHVNormalVTex VS(InstancePosNormalTex vIn)
{
    VertexPosHVNormalVTex vOut;
    
    vector posW = mul(float4(vIn.PosL, 1.0f), vIn.World);
    matrix worldView = mul(vIn.World, g_View);
    matrix worldInvTransposeView = mul(vIn.WorldInvTranspose, g_View);
    
    // 变换到观察空间
    vOut.PosV = mul(float4(vIn.PosL, 1.0f), worldView).xyz;
    vOut.NormalV = mul(vIn.NormalL, (float3x3) worldInvTransposeView);
    
    // 变换到裁剪空间
    vOut.PosH = mul(posW, g_ViewProj);
    
    vOut.Tex = vIn.Tex;
    
	return vOut;
}
