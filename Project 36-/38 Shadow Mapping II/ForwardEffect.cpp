#include "Effects.h"
#include "d3dUtil.h"
#include "RenderStates.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
#include "Material.h"
#include "MeshData.h"
#include "TextureManager.h"

using namespace DirectX;

# pragma warning(disable: 26812)


//
// ForwardEffect::Impl 需要先于ForwardEffect的定义
//

class ForwardEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::unique_ptr<EffectHelper> m_pEffectHelper;

	std::shared_ptr<IEffectPass> m_pCurrEffectPass;

	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

	XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// ForwardEffect
//

namespace
{
	// ForwardEffect单例
	static ForwardEffect * g_pInstance = nullptr;
}

ForwardEffect::ForwardEffect()
{
	if (g_pInstance)
		throw std::exception("ForwardEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<ForwardEffect::Impl>();
}

ForwardEffect::~ForwardEffect()
{
}

ForwardEffect::ForwardEffect(ForwardEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

ForwardEffect & ForwardEffect::operator=(ForwardEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

ForwardEffect & ForwardEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("ForwardEffect needs an instance!");
	return *g_pInstance;
}


bool ForwardEffect::InitAll(ID3D11Device * device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	// ******************
	// 创建顶点着色器
	//
	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryVS", L"Shaders\\Forward.hlsl", 
		device, "GeometryVS", "vs_5_0", nullptr, blob.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.ReleaseAndGetAddressOf()));

	// ******************
	// 创建像素着色器
	//

	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ForwardPS", L"Shaders\\Forward.hlsl",
		device, "ForwardPS", "ps_5_0", nullptr, blob.GetAddressOf()));


	// ******************
	// 创建通道
	//
	EffectPassDesc passDesc;
	passDesc.nameVS = "GeometryVS";
	passDesc.namePS = "ForwardPS";
	pImpl->m_pEffectHelper->AddEffectPass("Forward", device, &passDesc);
	{
		auto pPass = pImpl->m_pEffectHelper->GetEffectPass("Forward");
		// 注意：反向Z => GREATER_EQUAL测试
		pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
	}

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerDiffuse", RenderStates::SSAnistropicWrap16x.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "ForwardEffect.VertexPosNormalTexLayout");
	pImpl->m_pEffectHelper->SetDebugObjectName("ForwardEffect");

	return true;
}

void XM_CALLCONV ForwardEffect::SetShadowTransformMatrix(DirectX::FXMMATRIX S)
{
	XMMATRIX shadowTransform = XMMatrixTranspose(S);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowTransform")->SetFloatMatrix(4, 4, (const FLOAT*)&shadowTransform);
}

void ForwardEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Forward");
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


void XM_CALLCONV ForwardEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV ForwardEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV ForwardEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void ForwardEffect::SetMaterial(Material& material)
{
	TextureManager& tm = TextureManager::Get();

	const std::string& str = material.GetTexture("$Diffuse");
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureDiffuse", tm.GetTexture(str));
	if (auto prop = material.Get("$SpecularColor"); prop != nullptr)
		pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Kspecular")->Set(*prop);
}

MeshDataInput ForwardEffect::GetInputData(MeshData& meshData)
{
	MeshDataInput input;
	input.pVertexBuffers = {
		meshData.m_pVertices.Get(),
		meshData.m_pNormals.Get(),
		meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get()
	};
	input.strides = { 12, 12, 8 };
	input.offsets = { 0, 0, 0 };
	
	input.pIndexBuffer = meshData.m_pIndices.Get();
	input.indexCount = meshData.m_IndexCount;

	return input;
}

void ForwardEffect::SetLightPos(const DirectX::XMFLOAT3& pos)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightPosW")->SetFloatVector(3, &pos.x);
}

void ForwardEffect::SetLightIntensity(float intensity)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightIntensity")->SetFloat(intensity);
}

void ForwardEffect::SetSpecular(const DirectX::XMFLOAT4& specular)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Kspecular")->SetFloatVector(4, &specular.x);
}

void ForwardEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

	XMMATRIX WVP = W * V * P;
	XMMATRIX WInvT = XMath::InverseTranspose(W);

	W = XMMatrixTranspose(W);
	WVP = XMMatrixTranspose(WVP);
	WInvT = XMMatrixTranspose(WInvT);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (FLOAT*)&WInvT);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);

	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


