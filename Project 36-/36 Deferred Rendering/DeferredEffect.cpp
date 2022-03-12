#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)


//
// DeferredEffect::Impl 需要先于DeferredEffect的定义
//

class DeferredEffect::Impl
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
// DeferredEffect
//

namespace
{
	// DeferredEffect单例
	static DeferredEffect * g_pInstance = nullptr;
}

DeferredEffect::DeferredEffect()
{
	if (g_pInstance)
		throw std::exception("DeferredEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<DeferredEffect::Impl>();
}

DeferredEffect::~DeferredEffect()
{
}

DeferredEffect::DeferredEffect(DeferredEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

DeferredEffect & DeferredEffect::operator=(DeferredEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

DeferredEffect & DeferredEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("DeferredEffect needs an instance!");
	return *g_pInstance;
}


bool DeferredEffect::InitAll(ID3D11Device * device, UINT msaaSamples)
{
	pImpl->m_MsaaSamples = msaaSamples;

	if (!device)
		return false;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

	Microsoft::WRL::ComPtr<ID3DBlob> blob;
	std::string msaaSamplesStr = std::to_string(msaaSamples);
	D3D_SHADER_MACRO defines[] = {
		{"MSAA_SAMPLES", "1"},
		{nullptr, nullptr}
	};

	// ******************
	// 创建顶点着色器
	//

	HR(CreateShaderFromFile(nullptr, L"Shaders\\GBuffer.hlsl", defines, "GeometryVS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("GeometryVS", device, blob.Get()));
	// 创建顶点布局
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.ReleaseAndGetAddressOf()));

	HR(CreateShaderFromFile(nullptr, L"Shaders\\FullScreenTriangle.hlsl", defines, "FullScreenTriangleVS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(pImpl->m_pEffectHelper->AddShader("FullScreenTriangleVS", device, blob.Get()));

	int mssaSamples = 1;
	while (msaaSamples <= 8)
	{
		// ******************
		// 创建像素着色器
		//
		std::string msaaSamplesStr = std::to_string(msaaSamples);
		defines[0].Definition = msaaSamplesStr.c_str();
		std::string shaderNames[] = { 
			"GBuffer_" + msaaSamplesStr + "xMSAA_PS",
			"RequiresPerSampleShading_" + msaaSamplesStr + "xMSAA_PS",
			"BasicDeferred_" + msaaSamplesStr + "xMSAA_PS",
			"BasicDeferredPerSample_" + msaaSamplesStr + "xMSAA_PS",
			"DebugNormal" + msaaSamplesStr + "xMSAA_PS",
			"DebugPosZGrad" + msaaSamplesStr + "xMSAA_PS",
		};

		HR(CreateShaderFromFile(nullptr, L"Shaders\\GBuffer.hlsl", defines, "GBufferPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[0].c_str(), device, blob.Get()));

		HR(CreateShaderFromFile(nullptr, L"Shaders\\GBuffer.hlsl", defines, "RequiresPerSampleShadingPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[1].c_str(), device, blob.Get()));

		HR(CreateShaderFromFile(nullptr, L"Shaders\\BasicDeferred.hlsl", defines, "BasicDeferredPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[2].c_str(), device, blob.Get()));

		HR(CreateShaderFromFile(nullptr, L"Shaders\\BasicDeferred.hlsl", defines, "BasicDeferredPerSamplePS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[3].c_str(), device, blob.Get()));

		HR(CreateShaderFromFile(nullptr, L"Shaders\\GBuffer.hlsl", defines, "DebugNormalPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[4].c_str(), device, blob.Get()));

		HR(CreateShaderFromFile(nullptr, L"Shaders\\GBuffer.hlsl", defines, "DebugPosZGradPS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
		HR(pImpl->m_pEffectHelper->AddShader(shaderNames[5].c_str(), device, blob.Get()));

		// ******************
		// 创建通道
		//
		EffectPassDesc passDesc;
		passDesc.nameVS = "GeometryVS";
		passDesc.namePS = shaderNames[0].c_str();

		std::string passNames[] = {
			"GBuffer_" + msaaSamplesStr + "xMSAA",
			"Lighting_Basic_MaskStencil_" + msaaSamplesStr + "xMSAA",
			"Lighting_Basic_Deferred_PerPixel_" + msaaSamplesStr + "xMSAA",
			"Lighting_Basic_Deferred_PerSample_" + msaaSamplesStr + "xMSAA",
			"DebugNormal_" + msaaSamplesStr + "xMSAA",
			"DebugPosZGrad_" + msaaSamplesStr + "xMSAA"
		}; 

		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[0].c_str(), device, &passDesc));
		{
			auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passNames[0].c_str());
			// 注意：反向Z => GREATER_EQUAL测试
			pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
		}

		passDesc.nameVS = "FullScreenTriangleVS";
		passDesc.namePS = shaderNames[1].c_str();
		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[1].c_str(), device, &passDesc));
		{
			auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passNames[1].c_str());
			pPass->SetDepthStencilState(RenderStates::DSSWriteStencil.Get(), 1);
		}


		passDesc.nameVS = "FullScreenTriangleVS";
		passDesc.namePS = shaderNames[2].c_str();
		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[2].c_str(), device, &passDesc));
		{
			auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passNames[2].c_str());
			pPass->SetDepthStencilState(RenderStates::DSSEqualStencil.Get(), 0);
			pPass->SetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
		}

		passDesc.nameVS = "FullScreenTriangleVS";
		passDesc.namePS = shaderNames[3].c_str();
		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[3].c_str(), device, &passDesc));
		{
			auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passNames[3].c_str());
			pPass->SetDepthStencilState(RenderStates::DSSEqualStencil.Get(), 1);
			pPass->SetBlendState(RenderStates::BSAdditive.Get(), nullptr, 0xFFFFFFFF);
		}

		passDesc.nameVS = "FullScreenTriangleVS";
		passDesc.namePS = shaderNames[4].c_str();
		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[4].c_str(), device, &passDesc));

		passDesc.nameVS = "FullScreenTriangleVS";
		passDesc.namePS = shaderNames[5].c_str();
		HR(pImpl->m_pEffectHelper->AddEffectPass(passNames[5].c_str(), device, &passDesc));

		msaaSamples <<= 1;
	}
	
	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerDiffuse", RenderStates::SSAnistropicWrap16x.Get());

	// 设置调试对象名
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "DeferredEffect.VertexPosNormalTexLayout");
	pImpl->m_pEffectHelper->SetDebugObjectName("DeferredEffect");

	return true;
}

void DeferredEffect::SetMsaaSamples(UINT msaaSamples)
{
	pImpl->m_MsaaSamples = msaaSamples;
}

void DeferredEffect::SetLightingOnly(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_LightingOnly")->SetUInt(enable);
}

void DeferredEffect::SetFaceNormals(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FaceNormals")->SetUInt(enable);
}

void DeferredEffect::SetVisualizeLightCount(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizeLightCount")->SetUInt(enable);
}

void DeferredEffect::SetVisualizeShadingFreq(bool enable)
{
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizePerSampleShading")->SetUInt(enable);
}

void DeferredEffect::SetCameraNearFar(float nearZ, float farZ)
{
	float nearFar[4] = { nearZ, farZ };
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CameraNearFar")->SetFloatVector(4, nearFar);
}

void DeferredEffect::SetRenderGBuffer(ID3D11DeviceContext* deviceContext)
{
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	std::string gBufferPassStr = "GBuffer_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA";
	pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass(gBufferPassStr.c_str());

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DeferredEffect::DebugNormalGBuffer(ID3D11DeviceContext* deviceContext, 
	ID3D11RenderTargetView* rtv,
	ID3D11ShaderResourceView* normalGBuffer,
	D3D11_VIEWPORT viewport)
{
	// 设置全屏三角形
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	deviceContext->RSSetViewports(1, &viewport);
	std::string passStr = "DebugNormal_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA";
	pImpl->m_pEffectHelper->GetEffectPass(passStr.c_str())->Apply(deviceContext);
	deviceContext->PSSetShaderResources(1, 1, &normalGBuffer);
	deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
	deviceContext->Draw(3, 0);
	
	// 清空
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	normalGBuffer = nullptr;
	deviceContext->PSSetShaderResources(1, 1, &normalGBuffer);
}

void DeferredEffect::DebugPosZGradGBuffer(ID3D11DeviceContext* deviceContext, 
	ID3D11RenderTargetView* rtv, 
	ID3D11ShaderResourceView* posZGradGBuffer, 
	D3D11_VIEWPORT viewport)
{
	// 设置全屏三角形
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	deviceContext->RSSetViewports(1, &viewport);
	std::string passStr = "DebugPosZGrad_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA";
	pImpl->m_pEffectHelper->GetEffectPass(passStr.c_str())->Apply(deviceContext);
	deviceContext->PSSetShaderResources(3, 1, &posZGradGBuffer);
	deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
	deviceContext->Draw(3, 0);

	// 清空
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	posZGradGBuffer = nullptr;
	deviceContext->PSSetShaderResources(3, 1, &posZGradGBuffer);
}

void DeferredEffect::ComputeLightingDefault(
	ID3D11DeviceContext* deviceContext,
	ID3D11RenderTargetView* litBufferRTV,
	ID3D11DepthStencilView* depthBufferReadOnlyDSV,
	ID3D11ShaderResourceView* lightBufferSRV,
	ID3D11ShaderResourceView* GBuffers[4],
	D3D11_VIEWPORT viewport)
{
	std::string passStrs[] = {
		"Lighting_Basic_MaskStencil_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA",
		"Lighting_Basic_Deferred_PerPixel_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA",
		"Lighting_Basic_Deferred_PerSample_" + std::to_string(pImpl->m_MsaaSamples) + "xMSAA"
	};

	// 清屏
	const float zeros[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->ClearRenderTargetView(litBufferRTV, zeros);
	// 设置全屏三角形
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	deviceContext->RSSetViewports(1, &viewport);
	// 使用模板标记出需要应用逐样本着色的区域
	if (pImpl->m_MsaaSamples > 1)
	{
		pImpl->m_pEffectHelper->GetEffectPass(passStrs[0].c_str())->Apply(deviceContext);
		deviceContext->PSSetShaderResources(1, 4, GBuffers);
		deviceContext->PSSetShaderResources(5, 1, &lightBufferSRV);
		deviceContext->OMSetRenderTargets(0, 0, depthBufferReadOnlyDSV);
		deviceContext->Draw(3, 0);
	}
	
	// 通过模板测试来绘制逐像素着色的区域
	ID3D11RenderTargetView* pRTVs[1] = { litBufferRTV };
	deviceContext->OMSetRenderTargets(1, pRTVs, depthBufferReadOnlyDSV);

	pImpl->m_pEffectHelper->GetEffectPass(passStrs[1].c_str())->Apply(deviceContext);
	deviceContext->PSSetShaderResources(1, 4, GBuffers);
	deviceContext->PSSetShaderResources(5, 1, &lightBufferSRV);
	deviceContext->Draw(3, 0);

	// 通过模板测试来绘制逐样本着色的区域
	if (pImpl->m_MsaaSamples > 1)
	{
		pImpl->m_pEffectHelper->GetEffectPass(passStrs[2].c_str())->Apply(deviceContext);
		deviceContext->PSSetShaderResources(1, 4, GBuffers);
		deviceContext->PSSetShaderResources(5, 1, &lightBufferSRV);
		deviceContext->Draw(3, 0);
	}

	// 清空
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	ID3D11ShaderResourceView* nullSRVs[8]{};
	deviceContext->VSSetShaderResources(0, 8, nullSRVs);
	deviceContext->PSSetShaderResources(0, 8, nullSRVs);
}

void XM_CALLCONV DeferredEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV DeferredEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
	XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV DeferredEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
	XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void DeferredEffect::SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse)
{
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureDiffuse", textureDiffuse);
}

void DeferredEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
	XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
	XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

	XMMATRIX WV = W * V;
	XMMATRIX WVP = WV * P;
	XMMATRIX InvV = XMMatrixInverse(nullptr, V);
	XMMATRIX WInvTV = XMath::InverseTranspose(W) * V;
	XMMATRIX VP = V * P;

	WV = XMMatrixTranspose(WV);
	WVP = XMMatrixTranspose(WVP);
	InvV = XMMatrixTranspose(InvV);
	WInvTV = XMMatrixTranspose(WInvTV);
	P = XMMatrixTranspose(P);
	VP = XMMatrixTranspose(VP);

	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTransposeView")->SetFloatMatrix(4, 4, (FLOAT*)&WInvTV);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (FLOAT*)&WV);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InvView")->SetFloatMatrix(4, 4, (FLOAT*)&InvV);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (FLOAT*)&P);

	if (pImpl->m_pCurrEffectPass)
		pImpl->m_pCurrEffectPass->Apply(deviceContext);
}


