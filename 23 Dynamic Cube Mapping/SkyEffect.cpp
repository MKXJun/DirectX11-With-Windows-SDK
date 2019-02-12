#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// SkyEffect::Impl 需要先于SkyEffect的定义
//

class SkyEffect::Impl : public AlignedType<SkyEffect::Impl>
{
public:
	//
	// 这些结构体对应HLSL的结构体，仅供该文件使用。需要按16字节对齐
	//

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX worldViewProj;
	};

public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame>	cbFrame;	// 每帧绘制的常量缓冲区

	BOOL isDirty;										// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;				// 统一管理上面所有的常量缓冲区

	ComPtr<ID3D11VertexShader> skyVS;
	ComPtr<ID3D11PixelShader> skyPS;

	ComPtr<ID3D11InputLayout> vertexPosLayout;

	ComPtr<ID3D11ShaderResourceView> textureCube;			// 天空盒纹理
};

//
// SkyEffect
//

namespace
{
	// SkyEffect单例
	static SkyEffect * pInstance = nullptr;
}

SkyEffect::SkyEffect()
{
	if (pInstance)
		throw std::exception("BasicEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<SkyEffect::Impl>();
}

SkyEffect::~SkyEffect()
{
}

SkyEffect::SkyEffect(SkyEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

SkyEffect & SkyEffect::operator=(SkyEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

SkyEffect & SkyEffect::Get()
{
	if (!pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *pInstance;
}

bool SkyEffect::InitAll(ComPtr<ID3D11Device> device)
{
	if (!device)
		return false;

	if (!pImpl->cBufferPtrs.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;
	
	// ******************
	// 创建顶点着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\Sky_VS.cso", L"HLSL\\Sky_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->skyVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPos::inputLayout, ARRAYSIZE(VertexPos::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\Sky_PS.cso", L"HLSL\\Sky_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->skyPS.GetAddressOf()));


	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame,
	});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	return true;
}

void SkyEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexPosLayout.Get());
	deviceContext->VSSetShader(pImpl->skyVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->skyPS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSLessEqual.Get(), 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV SkyEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.worldViewProj = XMMatrixTranspose(W * V * P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV SkyEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX WVP)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.worldViewProj = XMMatrixTranspose(WVP);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void SkyEffect::SetTextureCube(ComPtr<ID3D11ShaderResourceView> textureCube)
{
	pImpl->textureCube = textureCube;
}

void SkyEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	
	// 设置SRV
	deviceContext->PSSetShaderResources(0, 1, pImpl->textureCube.GetAddressOf());

	if (pImpl->isDirty)
	{
		pImpl->isDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}

