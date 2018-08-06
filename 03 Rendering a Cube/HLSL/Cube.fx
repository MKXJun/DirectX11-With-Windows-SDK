
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
