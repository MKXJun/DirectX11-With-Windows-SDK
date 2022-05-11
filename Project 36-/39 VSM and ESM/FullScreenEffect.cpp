#include "Effects.h"
#include "XUtil.h"
#include "RenderStates.h"
#include "EffectHelper.h"
#include "DXTrace.h"
#include "Vertex.h"
#include "TextureManager.h"

using namespace DirectX;
using namespace Microsoft::WRL;

# pragma warning(disable: 26812)

//
// FullScreenEffect::Impl 需要先于FullScreenEffect的定义
//

class FullScreenEffect::Impl
{
public:
	// 必须显式指定
	Impl() {}
	~Impl() = default;

public:
	std::unique_ptr<EffectHelper> m_pEffectHelper;
};

//
// FullScreenEffect
//

namespace
{
	// FullScreenEffect单例
	static FullScreenEffect* g_pInstance = nullptr;
}

FullScreenEffect::FullScreenEffect()
{
	if (g_pInstance)
		throw std::exception("FullScreenEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<FullScreenEffect::Impl>();
}

FullScreenEffect::~FullScreenEffect()
{
}

FullScreenEffect::FullScreenEffect(FullScreenEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

FullScreenEffect& FullScreenEffect::operator=(FullScreenEffect&& moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

FullScreenEffect& FullScreenEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("FullScreenEffect needs an instance!");
	return *g_pInstance;
}

bool FullScreenEffect::InitAll(ID3D11Device* device)
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

	HR(pImpl->m_pEffectHelper->CreateShaderFromFile("FullScreenTriangleTexcoordVS", L"Shaders\\FullScreenShaders.hlsl",
		device, "FullScreenTriangleTexcoordVS", "vs_5_0"));

	// ******************
	// 创建像素着色器
	//

	const char* strs[] = {
		"3", "5", "7", "9", "11", "13", "15"
	};
	D3D_SHADER_MACRO defines[2] = {
		"BLUR_KERNEL_SIZE", "3",
		nullptr, nullptr
	};
	EffectPassDesc passDesc;
	passDesc.nameVS = "FullScreenTriangleTexcoordVS";
	std::string passName;
	// 盒型滤波
	for (const char * str : strs)
	{
		defines[0].Definition = str;
		HR(pImpl->m_pEffectHelper->CreateShaderFromFile("PSBlurX", L"Shaders\\FullScreenShaders.hlsl",
			device, "PSBlurX", "ps_5_0", defines));
		HR(pImpl->m_pEffectHelper->CreateShaderFromFile("PSBlurY", L"Shaders\\FullScreenShaders.hlsl",
			device, "PSBlurY", "ps_5_0", defines));

		// ******************
		// 创建通道
		//
		passDesc.namePS = "PSBlurX";
		passName = "BlurX_";
		passName += str;
		HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));

		passDesc.namePS = "PSBlurY";
		passName[4] = 'Y';
		HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));
	}

	// log高斯滤波
	for (auto str : strs)
	{
		defines[0].Definition = str;
		HR(pImpl->m_pEffectHelper->CreateShaderFromFile("PSLogGaussianBlur", L"Shaders\\FullScreenShaders.hlsl",
			device, "PSLogGaussianBlur", "ps_5_0", defines));

		// ******************
		// 创建通道
		//
		passDesc.namePS = "PSLogGaussianBlur";
		passName = "LogGaussianBlur_";
		passName += str;
		HR(pImpl->m_pEffectHelper->AddEffectPass(passName, device, &passDesc));
	}


	pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerPointClamp", RenderStates::SSPointClamp.Get());

	pImpl->m_pEffectHelper->SetDebugObjectName("FullScreenEffect");

	return true;
}

void FullScreenEffect::BlurX(ID3D11DeviceContext* deviceContext, 
	ID3D11ShaderResourceView* input, 
	ID3D11RenderTargetView* output, 
	const D3D11_VIEWPORT& vp, 
	int kernel_size)
{
	if (kernel_size % 2 == 0)
		return;

	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::string passName = "BlurX_" + std::to_string(kernel_size);
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", input);
	pPass->Apply(deviceContext);
	deviceContext->OMSetRenderTargets(1, &output, nullptr);
	deviceContext->RSSetViewports(1, &vp);
	deviceContext->Draw(3, 0);

	int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
	input = nullptr;
	deviceContext->PSSetShaderResources(slot, 1, &input);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FullScreenEffect::BlurY(ID3D11DeviceContext* deviceContext, 
	ID3D11ShaderResourceView* input, 
	ID3D11RenderTargetView* output, 
	const D3D11_VIEWPORT& vp, 
	int kernel_size)
{
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::string passName = "BlurY_" + std::to_string(kernel_size);
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", input);
	pPass->Apply(deviceContext);
	deviceContext->OMSetRenderTargets(1, &output, nullptr);
	deviceContext->RSSetViewports(1, &vp);
	deviceContext->Draw(3, 0);

	int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
	input = nullptr;
	deviceContext->PSSetShaderResources(slot, 1, &input);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FullScreenEffect::GaussianBlurX(
	ID3D11DeviceContext* deviceContext, 
	ID3D11ShaderResourceView* input, 
	ID3D11RenderTargetView* output, 
	const D3D11_VIEWPORT& vp, 
	int kernel_size)
{
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::string passName = "GaussianBlur_" + std::to_string(kernel_size);
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", input);
	pPass->Apply(deviceContext);
	deviceContext->OMSetRenderTargets(1, &output, nullptr);
	deviceContext->RSSetViewports(1, &vp);
	deviceContext->Draw(3, 0);

	int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
	input = nullptr;
	deviceContext->PSSetShaderResources(slot, 1, &input);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FullScreenEffect::GaussianBlurY(
	ID3D11DeviceContext* deviceContext, 
	ID3D11ShaderResourceView* input, 
	ID3D11RenderTargetView* output, 
	const D3D11_VIEWPORT& vp, 
	int kernel_size)
{
	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::string passName = "GaussianBlurY_" + std::to_string(kernel_size);
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", input);
	pPass->Apply(deviceContext);
	deviceContext->OMSetRenderTargets(1, &output, nullptr);
	deviceContext->RSSetViewports(1, &vp);
	deviceContext->Draw(3, 0);

	int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
	input = nullptr;
	deviceContext->PSSetShaderResources(slot, 1, &input);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FullScreenEffect::LogGaussianBlur(
	ID3D11DeviceContext* deviceContext, 
	ID3D11ShaderResourceView* input,
	ID3D11RenderTargetView* output, 
	const D3D11_VIEWPORT& vp, 
	int kernel_size,
	float sigma)
{
	float twoSigmaSq = 2.0f * sigma * sigma;

	float sum = 0.0f;
	float weights[16]{};
	int beg = -kernel_size / 2;
	int ed = kernel_size / 2;
	for (int i = beg; i <= ed; ++i)
	{
		float x = (float)i;
		weights[i - beg] = std::exp(-x * x / twoSigmaSq);
		sum += weights[i - beg];
	}

	// 标准化权值使得权值和为1.0
	for (int i = beg; i <= ed; ++i)
	{
		weights[i - beg] /= sum;
	}

	deviceContext->IASetInputLayout(nullptr);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	std::string passName = "LogGaussianBlur_" + std::to_string(kernel_size);
	auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
	pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", input);
	pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurWeightsArray")->SetRaw(weights);
	pPass->Apply(deviceContext);
	deviceContext->OMSetRenderTargets(1, &output, nullptr);
	deviceContext->RSSetViewports(1, &vp);
	deviceContext->Draw(3, 0);

	int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
	input = nullptr;
	deviceContext->PSSetShaderResources(slot, 1, &input);
	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void FullScreenEffect::Apply(ID3D11DeviceContext* deviceContext)
{
}

