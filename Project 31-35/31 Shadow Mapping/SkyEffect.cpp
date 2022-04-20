#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;


//
// SkyEffect::Impl 需要先于SkyEffect的定义
//

class SkyEffect::Impl
{

public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	std::unique_ptr<EffectHelper> m_pEffectHelper;
	std::shared_ptr<IEffectPass> m_pCurrEffectPass;

	ComPtr<ID3D11InputLayout> m_pVertexPosLayout;

	XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// SkyEffect
//

namespace
{
	// SkyEffect单例
	static SkyEffect * g_pInstance = nullptr;
}

SkyEffect::SkyEffect()
{
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<SkyEffect::Impl>();
}

SkyEffect::~SkyEffect()
{
}

SkyEffect::SkyEffect(SkyEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

SkyEffect & SkyEffect::operator=(SkyEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

SkyEffect & SkyEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *g_pInstance;
}

bool SkyEffect::InitAll(ID3D11Device * device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	ComPtr<ID3DBlob> blob;
	
	// ******************
	// 创建顶点着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\Sky_VS.cso", L"HLSL\\Sky_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("Sky_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPos::inputLayout, ARRAYSIZE(VertexPos::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\Sky_PS.cso", L"HLSL\\Sky_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("Sky_PS", device, blob.Get()));

	// ******************
	// 创建通道
	//
	EffectPassDesc passDesc;
	passDesc.nameVS = "Sky_VS";
	passDesc.namePS = "Sky_PS";
	HR(pImpl->m_pEffectHelper->AddEffectPass("Sky", device, &passDesc));

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Sky");
	pImpl->m_pCurrEffectPass->SetDepthStencilState(RenderStates::DSSLessEqual.Get(), 0);
	pImpl->m_pCurrEffectPass->SetRasterizerState(RenderStates::RSNoCull.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pVertexPosLayout.Get(), "SkyEffect.VertexPosLayout");
	pImpl->m_pEffectHelper->SetDebugObjectName("SkyEffect");

	return true;
}

void SkyEffect::SetRenderDefault(ID3D11DeviceContext * deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void XM_CALLCONV SkyEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV SkyEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV SkyEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void SkyEffect::SetTextureCube(ID3D11ShaderResourceView * m_pTextureCube)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TexCube", m_pTextureCube);
}

void SkyEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	XMMATRIX WVP = XMLoadFloat4x4(&pImpl->m_World) * XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
	WVP = XMMatrixTranspose(WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);

	pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

