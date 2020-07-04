#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)

//
// ShadowEffect::Impl 需要先于ShadowEffect的定义
//

class ShadowEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	std::unique_ptr<EffectHelper> m_pEffectHelper;

	std::shared_ptr<IEffectPass> m_pCurrEffectPass;

	ComPtr<ID3D11InputLayout> m_pInstancePosNormalTexLayout;
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	XMFLOAT4X4 m_World, m_View, m_Proj;
	ShadowEffect::RenderType m_RenderType;
};

//
// ShadowEffect
//

namespace
{
	// ShadowEffect单例
	static ShadowEffect* g_pInstance = nullptr;
}

ShadowEffect::ShadowEffect()
{
	if (g_pInstance)
		throw std::exception("ShadowEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<ShadowEffect::Impl>();
}

ShadowEffect::~ShadowEffect()
{
}

ShadowEffect::ShadowEffect(ShadowEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

ShadowEffect& ShadowEffect::operator=(ShadowEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

ShadowEffect& ShadowEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("ShadowEffect needs an instance!");
	return *g_pInstance;
}

bool ShadowEffect::InitAll(ID3D11Device* device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	ComPtr<ID3DBlob> blob;

	// 实例输入布局
	D3D11_INPUT_ELEMENT_DESC shadowInstLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};

	// ******************
	// 创建顶点着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\ShadowInstance_VS.cso", L"HLSL\\ShadowInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("ShadowInstance_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(shadowInstLayout, ARRAYSIZE(shadowInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\ShadowObject_VS.cso", L"HLSL\\ShadowObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("ShadowObject_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\Shadow_PS.cso", L"HLSL\\Shadow_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("Shadow_PS", device, blob.Get()));

	// ******************
	// 创建通道
	//
	EffectPassDesc passDesc;
	passDesc.nameVS = "ShadowInstance_VS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("ShadowInstance", device, &passDesc));
	pImpl->m_pEffectHelper->GetEffectPass("ShadowInstance")->SetRasterizerState(RenderStates::RSDepth.Get());

	passDesc.nameVS = "ShadowObject_VS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("ShadowObject", device, &passDesc));
	pImpl->m_pEffectHelper->GetEffectPass("ShadowObject")->SetRasterizerState(RenderStates::RSDepth.Get());

	passDesc.nameVS = "ShadowInstance_VS";
	passDesc.namePS = "Shadow_PS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("ShadowInstanceAlphaClip", device, &passDesc));
	pImpl->m_pEffectHelper->GetEffectPass("ShadowInstanceAlphaClip")->SetRasterizerState(RenderStates::RSDepth.Get());

	passDesc.nameVS = "ShadowObject_VS";
	passDesc.namePS = "Shadow_PS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("ShadowObjectAlphaClip", device, &passDesc));
	pImpl->m_pEffectHelper->GetEffectPass("ShadowObjectAlphaClip")->SetRasterizerState(RenderStates::RSDepth.Get());

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTexLayout.Get(), "ShadowEffect.InstancePosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "ShadowEffect.VertexPosNormalTexLayout");
	pImpl->m_pEffectHelper->SetDebugObjectName("ShadowEffect");

	return true;
}

void ShadowEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext, RenderType type)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ShadowInstance");
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ShadowObject");
	}
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pImpl->m_RenderType = type;
}

void ShadowEffect::SetRenderAlphaClip(ID3D11DeviceContext* deviceContext, RenderType type)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ShadowInstanceAlphaClip");
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ShadowObjectAlphaClip");
	}
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pImpl->m_RenderType = type;
}

void ShadowEffect::SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", textureDiffuse);
}

void XM_CALLCONV ShadowEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV ShadowEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV ShadowEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void ShadowEffect::Apply(ID3D11DeviceContext* deviceContext)
{
	if (pImpl->m_RenderType == RenderInstance)
	{
		XMMATRIX VP = XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
		VP = XMMatrixTranspose(VP);
		pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&VP);
	}
	else
	{
		XMMATRIX WVP = XMLoadFloat4x4(&pImpl->m_World) * XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
		WVP = XMMatrixTranspose(WVP);
		pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
	}

	pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

