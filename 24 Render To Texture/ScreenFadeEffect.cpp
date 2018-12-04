#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// ScreenFadeEffect::Impl 需要先于ScreenFadeEffect的定义
//

class ScreenFadeEffect::Impl : public AlignedType<ScreenFadeEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryFrame
	{
		float fadeAmount;
		XMFLOAT3 pad;
	};

	struct CBChangesRarely
	{
		XMMATRIX worldViewProj;
	};


public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame> cbFrame;		// 每帧修改的常量缓冲区
	CBufferObject<1, CBChangesRarely>	cbRarely;		// 很少修改的常量缓冲区
	

	BOOL isDirty;										// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;				// 统一管理上面所有的常量缓冲区

	ComPtr<ID3D11VertexShader> screenFadeVS;
	ComPtr<ID3D11PixelShader> screenFadePS;

	ComPtr<ID3D11InputLayout> vertexPosNormalTexLayout;

	ComPtr<ID3D11ShaderResourceView> texture;			// 用于淡入淡出的纹理
};



//
// ScreenFadeEffect
//

namespace
{
	// ScreenFadeEffect单例
	static ScreenFadeEffect * pInstance = nullptr;
}

ScreenFadeEffect::ScreenFadeEffect()
{
	if (pInstance)
		throw std::exception("ScreenFadeEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<ScreenFadeEffect::Impl>();
}

ScreenFadeEffect::~ScreenFadeEffect()
{
}

ScreenFadeEffect::ScreenFadeEffect(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

ScreenFadeEffect & ScreenFadeEffect::operator=(ScreenFadeEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

ScreenFadeEffect & ScreenFadeEffect::Get()
{
	if (!pInstance)
		throw std::exception("ScreenFadeEffect needs an instance!");
	return *pInstance;
}

bool ScreenFadeEffect::InitAll(ComPtr<ID3D11Device> device)
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

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_VS.cso", L"HLSL\\ScreenFade_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->screenFadeVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\ScreenFade_PS.cso", L"HLSL\\ScreenFade_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->screenFadePS.GetAddressOf()));



	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame,
		&pImpl->cbRarely
		});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void ScreenFadeEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->screenFadeVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->screenFadePS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(W * V * P);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV ScreenFadeEffect::SetWorldViewProjMatrix(DirectX::FXMMATRIX WVP)
{
	auto& cBuffer = pImpl->cbRarely;
	cBuffer.data.worldViewProj = XMMatrixTranspose(WVP);
	pImpl->isDirty = cBuffer.isDirty = true;
}

void ScreenFadeEffect::SetFadeAmount(float fadeAmount)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.fadeAmount = fadeAmount;
	pImpl->isDirty = cBuffer.isDirty = true;
}


void ScreenFadeEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->texture = texture;
}

void ScreenFadeEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	// 设置SRV
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
