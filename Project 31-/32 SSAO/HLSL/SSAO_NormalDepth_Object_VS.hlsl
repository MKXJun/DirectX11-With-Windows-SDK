#include "SSAO.hlsli"

// 生成观察空间的法向量和深度值的RTT的顶点着色器
VertexPosHVNormalVTex VS(VertexPosNormalTex vIn)
{
    VertexPosHVNormalVTex vOut;
    
    // 变换到观察空间
    vOut.PosV = mul(float4(vIn.PosL, 1.0f), g_WorldView).xyz;
    vOut.NormalV = mul(vIn.NormalL, (float3x3) g_WorldInvTransposeView);
    
    // 变换到裁剪空间
    vOut.PosH = mul(float4(vIn.PosL, 1.0f), g_WorldViewProj);
    
    vOut.Tex = vIn.Tex;
    
	return vOut;
}
