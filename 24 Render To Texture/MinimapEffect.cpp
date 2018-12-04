#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "Vertex.h"
using namespace DirectX;
using namespace std::experimental;


//
// MinimapEffect::Impl 需要先于MinimapEffect的定义
//

class MinimapEffect::Impl : public AlignedType<MinimapEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryFrame
	{
		XMVECTOR eyePos;
	};

	struct CBDrawingStates
	{
		int fogEnabled;
		float visibleRange;
		XMFLOAT2 pad;
		XMVECTOR rect;
		XMVECTOR invisibleColor;
	};


public:
	// 必须显式指定
	Impl() = default;
	~Impl() = default;

public:
	CBufferObject<0, CBChangesEveryFrame> cbFrame;		// 每帧修改的常量缓冲区
	CBufferObject<1, CBDrawingStates>	cbStates;		// 每次绘制状态改变的常量缓冲区


	BOOL isDirty;										// 是否有值变更
	std::vector<CBufferBase*> cBufferPtrs;				// 统一管理上面所有的常量缓冲区

	ComPtr<ID3D11VertexShader> minimapVS;
	ComPtr<ID3D11PixelShader> minimapPS;

	ComPtr<ID3D11InputLayout> vertexPosNormalTexLayout;

	ComPtr<ID3D11ShaderResourceView> texture;			// 用于淡入淡出的纹理
};



//
// MinimapEffect
//

namespace
{
	// MinimapEffect单例
	static MinimapEffect * pInstance = nullptr;
}

MinimapEffect::MinimapEffect()
{
	if (pInstance)
		throw std::exception("MinimapEffect is a singleton!");
	pInstance = this;
	pImpl = std::make_unique<MinimapEffect::Impl>();
}

MinimapEffect::~MinimapEffect()
{
}

MinimapEffect::MinimapEffect(MinimapEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
}

MinimapEffect & MinimapEffect::operator=(MinimapEffect && moveFrom)
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

MinimapEffect & MinimapEffect::Get()
{
	if (!pInstance)
		throw std::exception("MinimapEffect needs an instance!");
	return *pInstance;
}

bool MinimapEffect::InitAll(ComPtr<ID3D11Device> device)
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

	HR(CreateShaderFromFile(L"HLSL\\Minimap_VS.cso", L"HLSL\\Minimap_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->minimapVS.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->vertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\Minimap_PS.cso", L"HLSL\\Minimap_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->minimapPS.GetAddressOf()));


	pImpl->cBufferPtrs.assign({
		&pImpl->cbFrame,
		&pImpl->cbStates
		});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->cBufferPtrs)
	{
		pBuffer->CreateBuffer(device);
	}

	return true;
}

void MinimapEffect::SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->vertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->minimapVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(pImpl->minimapPS.Get(), nullptr, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	deviceContext->PSSetSamplers(0, 1, RenderStates::SSLinearWrap.GetAddressOf());
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);	// 关闭深度测试
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void MinimapEffect::SetFogState(bool isOn)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.fogEnabled = isOn;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void MinimapEffect::SetVisibleRange(float range)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.visibleRange = range;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetEyePos(DirectX::FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->cbFrame;
	cBuffer.data.eyePos = eyePos;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetMinimapRect(DirectX::FXMVECTOR rect)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.rect = rect;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV MinimapEffect::SetInvisibleColor(DirectX::FXMVECTOR color)
{
	auto& cBuffer = pImpl->cbStates;
	cBuffer.data.invisibleColor = color;
	pImpl->isDirty = cBuffer.isDirty = true;
}

void MinimapEffect::SetTexture(ComPtr<ID3D11ShaderResourceView> texture)
{
	pImpl->texture = texture;
}

void MinimapEffect::Apply(ComPtr<ID3D11DeviceContext> deviceContext)
{
	auto& pCBuffers = pImpl->cBufferPtrs;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
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
