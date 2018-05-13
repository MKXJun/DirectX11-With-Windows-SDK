
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

// 顶点着色器
VertexOut VS(VertexIn pIn)
{
	VertexOut pOut;
	pOut.posH = float4(pIn.pos, 1.0f);
	pOut.color = pIn.color;	// 这里alpha通道的值默认为1.0
    return pOut;
}


// 像素着色器
float4 PS(VertexOut pIn) : SV_Target
{
    return pIn.color;   
}
