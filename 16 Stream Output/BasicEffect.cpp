#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;




//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};

	struct CBChangesRarely
	{
		DirectionalLight dirLight[BasicEffect::maxLights];
		PointLight pointLight[BasicEffect::maxLights];
		SpotLight spotLight[BasicEffect::maxLights];
		Material material;
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT3 sphereCenter;
		float sphereRadius;
		DirectX::XMFLOAT3 eyePos;
		float pad;
	};

public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryFrame> cbFrame;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBChangesOnResize>   cbOnResize;	// 每次窗口大小变更的常量缓冲区
	CBufferObject<2, CBChangesRarely>     cbRarely;		// 几乎不会变更的常量缓冲区
	BOOL isDirty;										// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;				// 统一管理上面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> triangleSOVS;
	ComPtr<ID3D11GeometryShader> triangleSOGS;

	ComPtr<ID3D11VertexShader> triangleVS;
	ComPtr<ID3D11PixelShader> trianglePS;

	ComPtr<ID3D11VertexShader> sphereSOVS;
	ComPtr<ID3D11GeometryShader> sphereSOGS;

	ComPtr<ID3D11VertexShader> sphereVS;
	ComPtr<ID3D11PixelShader> spherePS;

	ComPtr<ID3D11VertexShader> snowSOVS;
	ComPtr<ID3D11GeometryShader> snowSOGS;

	ComPtr<ID3D11VertexShader> snowVS;
	ComPtr<ID3D11PixelShader> snowPS;

	ComPtr<ID3D11VertexShader> normalVS;
	ComPtr<ID3D11GeometryShader> normalGS;
	ComPtr<ID3D11PixelShader> normalPS;

	ComPtr<ID3D11InputLayout> vertexPosColorLayout;			// VertexPosColor输入布局
	ComPtr<ID3D11InputLayout> vertexPosNormalColorLayout;	// VertexPosNormalColor输入布局

	ComPtr<ID3D11ShaderResourceView> texture;				// 用于绘制的纹理

};

//
// BasicEffect
//

namespace
{
	// BasicEffect单例
	static BasicEffect * pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (pInstance)
		throw std::exception("BasicEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *pInstance;
}


bool BasicEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	const D3D11_SO_DECLARATION_ENTRY posColorLayout[2] = {
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};

	const D3D11_SO_DECLARATION_ENTRY posNormalColorLayout[3] = {
		{ 0, "POSITION", 0, 0, 3, 0 },
		{ 0, "NORMAL", 0, 0, 3, 0 },
		{ 0, "COLOR", 0, 0, 4, 0 }
	};

	UINT stridePosColor = sizeof(VertexPosColor);
	UINT stridePosNormalColor = sizeof(VertexPosNormalColor);

	ComPtr<ID3DBlob> blob;

	// ******************
	// 流输出分裂三角形
	//
	HR(CreateShaderFromFile(L"HLSL\\TriangleSO_VS.cso", L"HLSL\\TriangleSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosColorLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\TriangleSO_GS.cso", L"HLSL\\TriangleSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->triangleSOGS.GetAddressOf()));

	// ******************
	// 绘制分形三角形
	//
	HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso", L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso", L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->trianglePS.GetAddressOf()));


	// ******************
	// 流输出分形球体
	//
	HR(CreateShaderFromFile(L"HLSL\\SphereSO_VS.cso", L"HLSL\\SphereSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->sphereSOVS.GetAddressOf()));
	// 创建顶点输入布局
	HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout), blob->GetBufferPointer(),
		blob->GetBufferSize(), pImpl->vertexPosNormalColorLayout.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\SphereSO_GS.cso", L"HLSL\\SphereSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posNormalColorLayout, ARRAYSIZE(posNormalColorLayout),
		&stridePosNormalColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->sphereSOGS.GetAddressOf()));

	// ******************
	// 绘制球体
	//
	HR(CreateShaderFromFile(L"HLSL\\Sphere_VS.cso", L"HLSL\\Sphere_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->sphereVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Sphere_PS.cso", L"HLSL\\Sphere_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->spherePS.GetAddressOf()));


	// ******************
	// 流输出分形雪花
	//
	HR(CreateShaderFromFile(L"HLSL\\SnowSO_VS.cso", L"HLSL\\SnowSO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowSOVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\SnowSO_GS.cso", L"HLSL\\SnowSO_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), posColorLayout, ARRAYSIZE(posColorLayout),
		&stridePosColor, 1, D3D11_SO_NO_RASTERIZED_STREAM, nullptr, pImpl->snowSOGS.GetAddressOf()));

	// ******************
	// 绘制雪花
	//
	HR(CreateShaderFromFile(L"HLSL\\Snow_VS.cso", L"HLSL\\Snow_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Snow_PS.cso", L"HLSL\\Snow_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->snowPS.GetAddressOf()));


	// ******************
	// 绘制法向量
	//
	HR(CreateShaderFromFile(L"HLSL\\Normal_VS.cso", L"HLSL\\Normal_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalVS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Normal_GS.cso", L"HLSL\\Normal_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalGS.GetAddressOf()));
	HR(CreateShaderFromFile(L"HLSL\\Normal_PS.cso", L"HLSL\\Normal_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalPS.GetAddressOf()));



	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void BasicEffect::SetRenderSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->VSSetShader(pImpl->triangleVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->trianglePS.Get(), nullptr, 0);
}

void BasicEffect::SetRenderSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->VSSetShader(pImpl->snowVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->snowPS.Get(), nullptr, 0);
}

void BasicEffect::SetRenderSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->sphereVS.Get(), nullptr, 0);
	// 关闭流输出
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->spherePS.Get(), nullptr, 0);

}

void BasicEffect::SetStreamOutputSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());

	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->triangleSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->triangleSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);
	;
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicEffect::SetStreamOutputSplitedSnow(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->snowSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->snowSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicEffect::SetStreamOutputSplitedSphere(ComPtr<ID3D11DeviceContext> deviceContext, ComPtr<ID3D11Buffer> vertexBufferIn, ComPtr<ID3D11Buffer> vertexBufferOut)
{
	// 先恢复流输出默认设置，防止顶点缓冲区同时绑定在输入和输出阶段
	UINT stride = sizeof(VertexPosNormalColor);
	UINT offset = 0;
	ID3D11Buffer * nullBuffer = nullptr;
	deviceContext->SOSetTargets(1, &nullBuffer, &offset);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->IASetVertexBuffers(0, 1, vertexBufferIn.GetAddressOf(), &stride, &offset);

	deviceContext->VSSetShader(pImpl->sphereSOVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->sphereSOGS.Get(), nullptr, 0);

	deviceContext->SOSetTargets(1, vertexBufferOut.GetAddressOf(), &offset);

	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(nullptr, nullptr, 0);

}

void BasicEffect::SetRenderNormal(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->normalVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->normalGS.Get(), nullptr, 0);
	ID3D11Buffer* bufferArray[1] = { nullptr };
	UINT offset = 0;
	deviceContext->SOSetTargets(1, bufferArray, &offset);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->normalPS.Get(), nullptr, 0);

}




void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置抵消
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->cbOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetWorldViewProjMatrix(FXMMATRIX W, CXMMATRIX V, CXMMATRIX P)
{
	pImpl->cbFrame.data.world = XMMatrixTranspose(W);
	pImpl->cbFrame.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置抵消
	pImpl->cbRarely.data.view = XMMatrixTranspose(V);
	pImpl->cbOnResize.data.proj = XMMatrixTranspose(P);

	auto& pCBuffers = pImpl->cBufferPtrs;
	pCBuffers[0]->isDirty = pCBuffers[1]->isDirty = pCBuffers[2]->isDirty = true;
	pImpl->isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.material = material;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbRarely;
	XMStoreFloat3(&cBuffer.data.eyePos, eyePos);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSphereCenter(const DirectX::XMFLOAT3 & center)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.sphereCenter = center;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSphereRadius(float radius)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.sphereRadius = radius;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void BasicEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);

	pCBuffers[0]->BindGS(deviceContext);
	pCBuffers[1]->BindGS(deviceContext);
	pCBuffers[2]->BindGS(deviceContext);
	
	pCBuffers[2]->BindPS(deviceContext);

	// 设置纹理
	deviceContext->PSSetShaderResources(0, 1, pImpl->texture.GetAddressOf());

	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}


