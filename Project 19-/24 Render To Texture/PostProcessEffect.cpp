#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include "LightHelper.h"
using namespace DirectX;


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
    // 创建顶点着色器
    pImpl->m_pEffectHelper->CreateShaderFromFile("MinimapVS", L"Shaders/Minimap_VS.cso", device);
    pImpl->m_pEffectHelper->CreateShaderFromFile("ScreenFadeVS", L"Shaders/ScreenFade_VS.cso", device);

    // 创建像素着色器
    pImpl->m_pEffectHelper->CreateShaderFromFile("MinimapPS", L"Shaders/Minimap_PS.cso", device);
    pImpl->m_pEffectHelper->CreateShaderFromFile("ScreenFadePS", L"Shaders/ScreenFade_PS.cso", device);

    // 创建通道
    EffectPassDesc passDesc;
    passDesc.nameVS = "MinimapVS";
    passDesc.namePS = "MinimapPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Minimap", device, &passDesc));

    passDesc.nameVS = "ScreenFadeVS";
    passDesc.namePS = "ScreenFadePS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("ScreenFade", device, &passDesc));


    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

    pImpl->m_pEffectHelper->SetDebugObjectName("PostProcessEffect");

    return true;
}

void PostProcessEffect::RenderScreenFade(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp, 
    float fadeAmount)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Tex", input);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("ScreenFade");
    pPass->PSGetParamByName("fadeAmount")->SetFloat(fadeAmount);
    pPass->Apply(deviceContext);
    deviceContext->Draw(3, 0);

    // 清空
    output = nullptr;
    input = nullptr;
    deviceContext->OMSetRenderTargets(0, &output, nullptr);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Tex"), 1, &input);
}

void PostProcessEffect::SetVisibleRange(float range)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisibleRange")->SetFloat(range);
}

void PostProcessEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, reinterpret_cast<const float*>(&eyePos));
}

void PostProcessEffect::SetMinimapRect(const DirectX::XMFLOAT4& rect)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_RectW")->SetFloatVector(4, reinterpret_cast<const float*>(&rect));
}

void PostProcessEffect::RenderMinimap(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Tex", input);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("Minimap");
    pPass->Apply(deviceContext);
    deviceContext->Draw(3, 0);

    // 清空
    output = nullptr;
    input = nullptr;
    deviceContext->OMSetRenderTargets(0, &output, nullptr);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Tex"), 1, &input);
}
