#include "Effects.h"
#include "XUtil.h"
#include "RenderStates.h"
#include "EffectHelper.h"
#include "DXTrace.h"
#include "Vertex.h"
#include "TextureManager.h"
#include "Shaders/ShaderDefines.h"
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

	UINT m_MsaaSamples = 1;
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

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	D3D_SHADER_MACRO defines[] = {
		{"MSAA_SAMPLES", "1"},
		{nullptr, nullptr}
	};


	// ******************
	// 创建顶点着色器
	//
	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryVS", L"Shaders\\Forward.hlsl",
		device, "GeometryVS", "vs_5_0", nullptr, blob.GetAddressOf()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.ReleaseAndGetAddressOf()));

	// ******************
	// 创建像素着色器
	//
	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ForwardPS", L"Shaders\\Forward.hlsl",
		device, "ForwardPS", "ps_5_0"));
	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ForwardPlusPS", L"Shaders\\Forward.hlsl",
		device, "ForwardPlusPS", "ps_5_0"));

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
	
	passDesc.namePS = "ForwardPlusPS";
	pImpl->m_pEffectHelper->AddEffectPass("ForwardPlus", device, &passDesc);
	{
		auto pPass = pImpl->m_pEffectHelper->GetEffectPass("ForwardPlus");
		// 注意：反向Z => GREATER_EQUAL测试
		pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
	}

	passDesc.namePS = "";
	pImpl->m_pEffectHelper->AddEffectPass("PreZ", device, &passDesc);
	{
		auto pPass = pImpl->m_pEffectHelper->GetEffectPass("PreZ");
		// 注意：反向Z => GREATER_EQUAL测试
		pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
	}

	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerDiffuse", RenderStates::SSAnistropicWrap16x.Get());

	// ******************
	// 创建计算着色器与通道
	//
	
	UINT msaaSamples = 1;
	passDesc.nameVS = "";
	passDesc.namePS = "";
	while (msaaSamples <= 8)
	{
		std::string msaaSamplesStr = std::to_string(msaaSamples);
		defines[0].Definition = msaaSamplesStr.c_str();
		std::string shaderName = "ComputeShaderTileForward" + msaaSamplesStr + "xMSAA_CS";
		HR(pImpl->m_pEffectHelper->CreateShaderFromFile(shaderName, L"Shaders\\ComputeShaderTile.hlsl",
			device, "ComputeShaderTileForwardCS", "cs_5_0", defines));

		passDesc.nameCS = shaderName.c_str();
		std::string passName = "ComputeShaderTileForward_" + std::to_string(msaaSamples) + "xMSAA";
		pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc);

		msaaSamples <<= 1;
	}

	// 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	std::string str = "ForwardEffect.VertexPosNormalTexLayout";
	pImpl->m_pVertexPosNormalTexLayout->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)str.length(), str.c_str());
#endif
	pImpl->m_pEffectHelper->SetDebugObjectName("ForwardEffect");

	return true;
}


void ForwardEffect::SetMsaaSamples(UINT msaaSamples)
{
	pImpl->m_MsaaSamples = msaaSamples;
}

void ForwardEffect::SetLightBuffer(ID3D11ShaderResourceView* lightBuffer)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_Light", lightBuffer);
}

void ForwardEffect::SetTileBuffer(ID3D11ShaderResourceView* tileBuffer)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_Tilebuffer", tileBuffer);
}

void ForwardEffect::SetCameraNearFar(float nearZ, float farZ)
{
	float nearFar[4] = { nearZ, farZ };
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CameraNearFar")->SetFloatVector(4, nearFar);
}

void ForwardEffect::SetLightingOnly(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightingOnly")->SetUInt(enable);
}

void ForwardEffect::SetFaceNormals(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FaceNormals")->SetUInt(enable);
}

void ForwardEffect::SetVisualizeLightCount(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizeLightCount")->SetUInt(enable);
}

void ForwardEffect::SetRenderPreZPass(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("PreZ");
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Forward");
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ForwardEffect::ComputeTiledLightCulling(ID3D11DeviceContext* deviceContext, 
	ID3D11UnorderedAccessView* tileInfoBufferUAV, 
	ID3D11ShaderResourceView* lightBufferSRV,
	ID3D11ShaderResourceView* depthBufferSRV)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pTex;
	depthBufferSRV->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.GetAddressOf()));
	D3D11_TEXTURE2D_DESC texDesc;
	pTex->GetDesc(&texDesc);

	UINT dims[2] = { texDesc.Width, texDesc.Height };
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FramebufferDimensions")->SetUIntVector(2, dims);
	pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_TilebufferRW", tileInfoBufferUAV, 0);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_Light", lightBufferSRV);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[3]", depthBufferSRV);
	
	std::string passName = "ComputeShaderTileForward_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA";
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pPass->Apply(deviceContext);

	// 调度
	UINT dispatchWidth = (texDesc.Width + COMPUTE_SHADER_TILE_GROUP_DIM - 1) / COMPUTE_SHADER_TILE_GROUP_DIM;
	UINT dispatchHeight = (texDesc.Height + COMPUTE_SHADER_TILE_GROUP_DIM - 1) / COMPUTE_SHADER_TILE_GROUP_DIM;
	deviceContext->Dispatch(dispatchWidth, dispatchHeight, 1);

	// 清空
	pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_TilebufferRW", nullptr, 0);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_Light", nullptr);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[3]", nullptr);
	pPass->Apply(deviceContext);
}

void ForwardEffect::SetRenderWithTiledLightCulling(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("ForwardPlus");
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

void ForwardEffect::SetMaterial(const Material& material)
{
	TextureManager& tm = TextureManager::Get();

	const std::string& str = material.GetTexture("$Diffuse");
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureDiffuse", tm.GetTexture(str));
}

MeshDataInput ForwardEffect::GetInputData(const MeshData& meshData)
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

void ForwardEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

	XMMATRIX WV = W * V;
	XMMATRIX WVP = WV * P;
	XMMATRIX WInvTV = XMath::InverseTranspose(W) * V;
	XMMATRIX VP = V * P;

	WV = XMMatrixTranspose(WV);
	WVP = XMMatrixTranspose(WVP);
	WInvTV = XMMatrixTranspose(WInvTV);
	P = XMMatrixTranspose(P);
	VP = XMMatrixTranspose(VP);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTransposeView")->SetFloatMatrix(4, 4, (FLOAT*)&WInvTV);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (FLOAT*)&WV);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (FLOAT*)&P);

	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


