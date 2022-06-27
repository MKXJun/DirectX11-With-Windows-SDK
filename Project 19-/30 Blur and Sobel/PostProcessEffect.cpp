#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>
#include <ModelManager.h>
#include "LightHelper.h"

using namespace DirectX;

# pragma warning(disable: 26812)


static void GenerateGaussianWeights(float weights[], int kernelSize, float sigma)
{
    float twoSigmaSq = 2.0f * sigma * sigma;
    int radius = kernelSize / 2;
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i)
    {
        float x = (float)i;

        weights[radius + i] = expf(-x * x / twoSigmaSq);

        sum += weights[radius + i];
    }

    // 标准化权值使得权值和为1.0
    for (int i = 0; i <= kernelSize; ++i)
    {
        weights[i] /= sum;
    }
}

//
// PostProcessEffect::Impl 需要先于PostProcessEffect的定义
//

class PostProcessEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    float m_Weights[32]{};
    int m_BlurRadius = 3;
    float m_BlurSigma = 1.0f;
    
};

//
// PostProcessEffect
//

namespace
{
    // PostProcessEffect单例
    static PostProcessEffect* g_pInstance = nullptr;
}

PostProcessEffect::PostProcessEffect()
{
    if (g_pInstance)
        throw std::exception("PostProcessEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<PostProcessEffect::Impl>();
}

PostProcessEffect::~PostProcessEffect()
{
}

PostProcessEffect::PostProcessEffect(PostProcessEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

PostProcessEffect& PostProcessEffect::operator=(PostProcessEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

PostProcessEffect& PostProcessEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("PostProcessEffect needs an instance!");
    return *g_pInstance;
}

bool PostProcessEffect::InitAll(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();


    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    // 创建计算着色器
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("BlurHorzCS", L"Shaders/Blur_Horz_CS.cso", device));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("BlurVertCS", L"Shaders/Blur_Vert_CS.cso", device));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("CompositeVS", L"Shaders/Composite_VS.cso", device));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("CompositePS", L"Shaders/Composite_PS.cso", device));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("BlurVertCS", L"Shaders/Blur_Vert_CS.cso", device));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SobelCS", L"Shaders/Sobel_CS.cso", device));


    // 创建通道
    EffectPassDesc passDesc;
    passDesc.nameCS = "BlurHorzCS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("BlurHorz", device, &passDesc));
    passDesc.nameCS = "BlurVertCS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("BlurVert", device, &passDesc));
    passDesc.nameCS = "SobelCS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Sobel", device, &passDesc));
    passDesc.nameVS = "CompositeVS";
    passDesc.namePS = "CompositePS";
    passDesc.nameCS = "";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Composite", device, &passDesc));


    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearWrap", RenderStates::SSLinearWrap.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamPointClamp", RenderStates::SSPointClamp.Get());

    pImpl->m_pEffectHelper->SetDebugObjectName("PostProcessEffect");

    return true;
}

void PostProcessEffect::RenderComposite(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input1,
    ID3D11ShaderResourceView* input2,
    ID3D11RenderTargetView* output,
    const D3D11_VIEWPORT& vp)
{
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("Composite");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Input", input1);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_EdgeInput", input2 ? input2 : TextureManager::Get().GetNullTexture());
    pPass->Apply(deviceContext);

    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->Draw(3, 0);

    // 清空
    input1 = nullptr;
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Input"), 1, &input1);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_EdgeInput"), 1, &input1);
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void PostProcessEffect::ComputeSobel(ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11UnorderedAccessView* output, 
    uint32_t width, uint32_t height)
{
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("Sobel");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Input", input);
    pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_Output", output);
    pPass->Apply(deviceContext);
    pPass->Dispatch(deviceContext, width, height);

    // 清空
    input = nullptr;
    output = nullptr;
    deviceContext->CSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Input"), 1, &input);
    deviceContext->CSSetUnorderedAccessViews(pImpl->m_pEffectHelper->MapUnorderedAccessSlot("g_Output"), 1, &output, nullptr);
}

void PostProcessEffect::SetBlurKernelSize(int size)
{
    if (size % 2 == 0 || size > ARRAYSIZE(pImpl->m_Weights))
        return;

    pImpl->m_BlurRadius = size / 2;
    GenerateGaussianWeights(pImpl->m_Weights, size, pImpl->m_BlurSigma);
}

void PostProcessEffect::SetBlurSigma(float sigma)
{
    if (sigma < 0.0f)
        return;

    pImpl->m_BlurSigma = sigma;
    GenerateGaussianWeights(pImpl->m_Weights, pImpl->m_BlurRadius * 2 + 1, pImpl->m_BlurSigma);
}

void PostProcessEffect::ComputeGaussianBlurX(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11UnorderedAccessView* output, 
    uint32_t width, uint32_t height)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("BlurHorz");
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Weights")->SetRaw(pImpl->m_Weights);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurRadius")->SetSInt(pImpl->m_BlurRadius);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Input", input);
    pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_Output", output);
    pPass->Apply(deviceContext);
    pPass->Dispatch(deviceContext, width, height);

    // 清空
    input = nullptr;
    output = nullptr;
    deviceContext->CSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Input"), 1, &input);
    deviceContext->CSSetUnorderedAccessViews(pImpl->m_pEffectHelper->MapUnorderedAccessSlot("g_Output"), 1, &output, nullptr);
}

void PostProcessEffect::ComputeGaussianBlurY(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11UnorderedAccessView* output,
    uint32_t width, uint32_t height)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("BlurVert");
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Weights")->SetRaw(pImpl->m_Weights);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurRadius")->SetSInt(pImpl->m_BlurRadius);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Input", input);
    pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_Output", output);
    pPass->Apply(deviceContext);
    pPass->Dispatch(deviceContext, width, height);

    // 清空
    input = nullptr;
    output = nullptr;
    deviceContext->CSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Input"), 1, &input);
    deviceContext->CSSetUnorderedAccessViews(pImpl->m_pEffectHelper->MapUnorderedAccessSlot("g_Output"), 1, &output, nullptr);
}
