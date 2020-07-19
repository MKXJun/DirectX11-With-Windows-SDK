#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)

//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl
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
	ComPtr<ID3D11InputLayout> m_pInstancePosNormalTangentTexLayout;
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTangentTexLayout;

	XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// BasicEffect
//

namespace
{
	// BasicEffect单例
	static BasicEffect * g_pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *g_pInstance;
}


bool BasicEffect::InitAll(ID3D11Device * device)
{
	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	ComPtr<ID3DBlob> blob;

	// 实例输入布局
	D3D11_INPUT_ELEMENT_DESC basicInstLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};

	D3D11_INPUT_ELEMENT_DESC normalmapInstLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		{ "WorldInvTranspose", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1}
	};

	// ******************
	// 创建顶点着色器
	//

	HR(CreateShaderFromFile(L"HLSL\\BasicInstance_VS.cso", L"HLSL\\BasicInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("BasicInstance_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(basicInstLayout, ARRAYSIZE(basicInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTexLayout.GetAddressOf()));


	HR(CreateShaderFromFile(L"HLSL\\BasicObject_VS.cso", L"HLSL\\BasicObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("BasicObject_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\NormalMapInstance_VS.cso", L"HLSL\\NormalMapInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("NormalMapInstance_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(normalmapInstLayout, ARRAYSIZE(normalmapInstLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTangentTexLayout.GetAddressOf()));
	
	HR(CreateShaderFromFile(L"HLSL\\NormalMapObject_VS.cso", L"HLSL\\NormalMapObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("NormalMapObject_VS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTangentTex::inputLayout, ARRAYSIZE(VertexPosNormalTangentTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTangentTexLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\DisplacementMapObject_VS.cso", L"HLSL\\DisplacementMapObject_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("DisplacementMapObject_VS", device, blob.Get()));

	HR(CreateShaderFromFile(L"HLSL\\DisplacementMapInstance_VS.cso", L"HLSL\\DisplacementMapInstance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("DisplacementMapInstance_VS", device, blob.Get()));

	// ******************
	// 创建外壳着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\DisplacementMap_HS.cso", L"HLSL\\DisplacementMap_HS.hlsl", "HS", "hs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("DisplacementMap_HS", device, blob.Get()));

	// ******************
	// 创建域着色器
	//
	HR(CreateShaderFromFile(L"HLSL\\DisplacementMap_DS.cso", L"HLSL\\DisplacementMap_DS.hlsl", "DS", "ds_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("DisplacementMap_DS", device, blob.Get()));

	// ******************
	// 创建像素着色器
	//
	
	HR(CreateShaderFromFile(L"HLSL\\Basic_PS.cso", L"HLSL\\Basic_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("Basic_PS", device, blob.Get()));

	HR(CreateShaderFromFile(L"HLSL\\NormalMap_PS.cso", L"HLSL\\NormalMap_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("NormalMap_PS", device, blob.Get()));

	// ******************
	// 创建通道
	//
	EffectPassDesc passDesc;
	passDesc.nameVS = "BasicObject_VS";
	passDesc.namePS = "Basic_PS";
	pImpl->m_pEffectHelper->AddEffectPass("BasicObject", device, &passDesc);
	passDesc.nameVS = "BasicInstance_VS";
	passDesc.namePS = "Basic_PS";
	pImpl->m_pEffectHelper->AddEffectPass("BasicInstance", device, &passDesc);
	passDesc.nameVS = "NormalMapObject_VS";
	passDesc.namePS = "NormalMap_PS";
	pImpl->m_pEffectHelper->AddEffectPass("NormalMapObject", device, &passDesc);
	passDesc.nameVS = "NormalMapInstance_VS";
	passDesc.namePS = "NormalMap_PS";
	pImpl->m_pEffectHelper->AddEffectPass("NormalMapInstance", device, &passDesc);
	passDesc.nameVS = "DisplacementMapObject_VS";
	passDesc.nameHS = "DisplacementMap_HS";
	passDesc.nameDS = "DisplacementMap_DS";
	passDesc.namePS = "NormalMap_PS";
	pImpl->m_pEffectHelper->AddEffectPass("DisplacementMapObject", device, &passDesc);
	passDesc.nameVS = "DisplacementMapInstance_VS";
	passDesc.nameHS = "DisplacementMap_HS";
	passDesc.nameDS = "DisplacementMap_DS";
	passDesc.namePS = "NormalMap_PS";
	pImpl->m_pEffectHelper->AddEffectPass("DisplacementMapInstance", device, &passDesc);

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());
	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamShadow", RenderStates::SSShadow.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTexLayout.Get(), "BasicEffect.InstancePosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "BasicEffect.VertexPosNormalTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTangentTexLayout.Get(), "BasicEffect.InstancePosNormalTangentTexLayout");
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTangentTexLayout.Get(), "BasicEffect.VertexPosNormalTangentTexLayout");
	pImpl->m_pEffectHelper->SetDebugObjectName("BasicEffect");
	
	return true;
}


void BasicEffect::SetRenderDefault(ID3D11DeviceContext * deviceContext, RenderType type, RSFillMode fillMode)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("BasicInstance");
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("BasicObject");
	}

	if (fillMode == Solid)
		pImpl->m_pCurrEffectPass->SetRasterizerState(nullptr);
	else
		pImpl->m_pCurrEffectPass->SetRasterizerState(RenderStates::RSWireframe.Get());

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void BasicEffect::SetRenderWithNormalMap(ID3D11DeviceContext* deviceContext, RenderType type, RSFillMode fillMode)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTangentTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("NormalMapInstance");
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTangentTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("NormalMapObject");
	}

	if (fillMode == Solid)
		pImpl->m_pCurrEffectPass->SetRasterizerState(nullptr);
	else
		pImpl->m_pCurrEffectPass->SetRasterizerState(RenderStates::RSWireframe.Get());

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void BasicEffect::SetRenderWithDisplacementMap(ID3D11DeviceContext* deviceContext, RenderType type, RSFillMode fillMode)
{
	if (type == RenderInstance)
	{
		deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTangentTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DisplacementMapInstance");
	}
	else
	{
		deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTangentTexLayout.Get());
		pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DisplacementMapObject");
	}

	if (fillMode == Solid)
		pImpl->m_pCurrEffectPass->SetRasterizerState(nullptr);
	else
		pImpl->m_pCurrEffectPass->SetRasterizerState(RenderStates::RSWireframe.Get());

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
}

void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV BasicEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV BasicEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void XM_CALLCONV BasicEffect::SetShadowTransformMatrix(DirectX::FXMMATRIX S)
{
	XMMATRIX shadowTransform = XMMatrixTranspose(S);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowTransform")->SetFloatMatrix(4, 4, (const FLOAT*)&shadowTransform);
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLight")->SetRaw(&dirLight, (UINT)pos * sizeof(dirLight), sizeof(dirLight));
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PointLight")->SetRaw(&pointLight, (UINT)pos * sizeof(pointLight), sizeof(pointLight));
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SpotLight")->SetRaw(&spotLight, (UINT)pos * sizeof(spotLight), sizeof(spotLight));
}

void BasicEffect::SetMaterial(const Material & material)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Material")->SetRaw(&material);
}

void BasicEffect::SetTextureUsed(bool isUsed)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TextureUsed")->SetSInt(isUsed);
}

void BasicEffect::SetShadowEnabled(bool enabled)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EnableShadow")->SetSInt(enabled);
}

void BasicEffect::SetSSAOEnabled(bool enabled)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EnableSSAO")->SetSInt(enabled);
	// 我们在绘制SSAO法向量/深度图的时候也已经写入了主要的深度/模板贴图，
	// 所以我们可以直接使用深度值相等的测试，这样可以避免在当前的一趟渲染中
	// 出现任何的重复写入当前像素的情况，只有距离最近的像素才会通过深度比较测试
	pImpl->m_pEffectHelper->GetEffectPass("BasicObject")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
	pImpl->m_pEffectHelper->GetEffectPass("BasicInstance")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
	pImpl->m_pEffectHelper->GetEffectPass("NormalMapObject")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
	pImpl->m_pEffectHelper->GetEffectPass("NormalMapInstance")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
	pImpl->m_pEffectHelper->GetEffectPass("DisplacementMapObject")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
	pImpl->m_pEffectHelper->GetEffectPass("DisplacementMapInstance")->SetDepthStencilState((enabled ? RenderStates::DSSEqual.Get() : nullptr), 0);
}

void BasicEffect::SetTextureDiffuse(ID3D11ShaderResourceView * textureDiffuse)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", textureDiffuse);
}

void BasicEffect::SetTextureNormalMap(ID3D11ShaderResourceView* textureNormalMap)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", textureNormalMap);
}

void BasicEffect::SetTextureShadowMap(ID3D11ShaderResourceView* textureShadowMap)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_ShadowMap", textureShadowMap);
}

void BasicEffect::SetTextureSSAOMap(ID3D11ShaderResourceView* textureSSAOMap)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_SSAOMap", textureSSAOMap);
}

void BasicEffect::SetTextureCube(ID3D11ShaderResourceView* textureCube)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TexCube", textureCube);
}

void BasicEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (FLOAT*)&eyePos);
}

void BasicEffect::SetHeightScale(float scale)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_HeightScale")->SetFloat(scale);
}

void BasicEffect::SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessDistance")->SetFloat(maxTessDistance);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessDistance")->SetFloat(minTessDistance);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessFactor")->SetFloat(minTessFactor);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessFactor")->SetFloat(maxTessFactor);
}

void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

	XMMATRIX WVP = W * V * P;
	XMMATRIX WInvT = InverseTranspose(W);
	XMMATRIX VP = V * P;

	WVP = XMMatrixTranspose(WVP);
	WInvT = XMMatrixTranspose(WInvT);
	W = XMMatrixTranspose(W);
	VP = XMMatrixTranspose(VP);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (FLOAT*)&W);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (FLOAT*)&WInvT);

	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


