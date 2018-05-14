
cbuffer ConstantBuffer : register(b0)
{
    row_major matrix World; // matrix 可以用float4x4替代。不加row_major的情况下，矩阵默认为列主矩阵，
    row_major matrix View;  // 在D3D传递给HLSL的过程中会发生转置。加了后，就变成行主矩阵。
    row_major matrix Proj;  
}


struct VertexIn
{
	float3 pos : POSITION;
	float4 color : COLOR;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VertexOut VS(VertexIn pIn)
{
	VertexOut pOut;
    pOut.posH = mul(float4(pIn.pos, 1.0f), World);      // mul 才是矩阵乘法, 运算符*要求
    pOut.posH = mul(pOut.posH, View);                   // 行列数相等的两个矩阵，结果为
    pOut.posH = mul(pOut.posH, Proj);                   // Cij = Aij * Bij
	pOut.color = pIn.color;	                            // 这里alpha通道的值默认为1.0
    return pOut;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VertexOut pIn) : SV_Target
{
    return pIn.color;   
}
