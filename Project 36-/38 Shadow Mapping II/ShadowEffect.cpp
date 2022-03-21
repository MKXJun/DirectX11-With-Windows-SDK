#include "Effects.h"
#include "XUtil.h"
#include "EffectHelper.h"
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
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	XMFLOAT4X4 m_World, m_View, m_Proj;
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

	Microsoft::WRL::ComPtr<ID3DBlob> blob;

	// ******************
	// 创建顶点着色器
	//

	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowVS", L"HLSL\\Shadow.hlsl", 
		device, "ShadowVS", "vs_5_0", nullptr, blob.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowPS", L"HLSL\\Shadow.hlsl",
		device, "ShadowPS", "ps_5_0"));

	// ******************
	// 创建通道
	//
	/*EffectPassDesc passDesc;
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
	pImpl->m_pEffectHelper->GetEffectPass("ShadowObjectAlphaClip")->SetRasterizerState(RenderStates::RSDepth.Get());*/

	//pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

	//// 设置调试对象名
	//D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTexLayout.Get(), "ShadowEffect.InstancePosNormalTexLayout");
	//D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "ShadowEffect.VertexPosNormalTexLayout");
	//pImpl->m_pEffectHelper->SetDebugObjectName("ShadowEffect");

	return true;
}

void ShadowEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ShadowObject");
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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
	XMMATRIX WVP = XMLoadFloat4x4(&pImpl->m_World) * XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
	WVP = XMMatrixTranspose(WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);

	pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

