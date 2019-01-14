#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
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
		DirectX::XMFLOAT3 eyePos;
		float cylinderHeight;
	};

public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryFrame>   cbFrame;		// 每次对象绘制的常量缓冲区
	CBufferObject<1, CBChangesOnResize>     cbOnResize;		// 每次窗口大小变更的常量缓冲区
	CBufferObject<2, CBChangesRarely>		cbRarely;		// 几乎不会变更的常量缓冲区
	BOOL isDirty;											// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;					// 统一管理上面所有的常量缓冲区


	ComPtr<ID3D11VertexShader> triangleVS;
	ComPtr<ID3D11PixelShader> trianglePS;
	ComPtr<ID3D11GeometryShader> triangleGS;

	ComPtr<ID3D11VertexShader> cylinderVS;
	ComPtr<ID3D11PixelShader> cylinderPS;
	ComPtr<ID3D11GeometryShader> cylinderGS;

	ComPtr<ID3D11VertexShader> normalVS;
	ComPtr<ID3D11PixelShader> normalPS;
	ComPtr<ID3D11GeometryShader> normalGS;

	ComPtr<ID3D11InputLayout> vertexPosColorLayout;		// VertexPosColor输入布局
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

	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器和顶点布局
	HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso", L"HLSL\\Triangle_VS.hlsl", "VS", "vs_5_0", blob.GetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleVS.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosColorLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Cylinder_VS.cso", L"HLSL\\Cylinder_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->cylinderVS.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosNormalColorLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Normal_VS.cso", L"HLSL\\Normal_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalVS.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso", L"HLSL\\Triangle_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->trianglePS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Cylinder_PS.cso", L"HLSL\\Cylinder_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->cylinderPS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Normal_PS.cso", L"HLSL\\Normal_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalPS.GetAddressOf()));

	// 创建几何着色器
	HR(CreateShaderFromFile(L"HLSL\\Triangle_GS.cso", L"HLSL\\Triangle_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->triangleGS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Cylinder_GS.cso", L"HLSL\\Cylinder_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->cylinderGS.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Normal_GS.cso", L"HLSL\\Normal_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->normalGS.GetAddressOf()));


	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame, 
		&pImpl->cbOnResize, 
		&pImpl->cbRarely});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void BasicEffect::SetRenderSplitedTriangle(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosColorLayout.Get());
	deviceContext->VSSetShader(pImpl->triangleVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->triangleGS.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->trianglePS.Get(), nullptr, 0);

}

void BasicEffect::SetRenderCylinderNoCap(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->cylinderVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->cylinderGS.Get(), nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());
	deviceContext->PSSetShader(pImpl->cylinderPS.Get(), nullptr, 0);

}

void BasicEffect::SetRenderNormal(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->normalVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->normalGS.Get(), nullptr, 0);
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

void BasicEffect::SetCylinderHeight(float height)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.cylinderHeight = height;
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






