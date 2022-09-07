#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
using namespace DirectX;

//
// SkyboxToneMapEffect::Impl 需要先于SkyboxEffect的定义
//


class SkyboxToneMapEffect::Impl
{

public:
    // 必须显式指定
    Impl() {
        XMStoreFloat4x4(&m_View, XMMatrixIdentity());
        XMStoreFloat4x4(&m_Proj, XMMatrixIdentity());
    }
    ~Impl() = default;

public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    XMFLOAT4X4 m_View, m_Proj;
};

//
// SkyboxToneMapEffect
//

namespace
{
    // SkyboxToneMapEffect单例
    static SkyboxToneMapEffect* g_pInstance = nullptr;
}

SkyboxToneMapEffect::SkyboxToneMapEffect()
{
    if (g_pInstance)
        throw std::exception("SkyboxToneMapEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<SkyboxToneMapEffect::Impl>();
}

SkyboxToneMapEffect::~SkyboxToneMapEffect()
{
}

SkyboxToneMapEffect::SkyboxToneMapEffect(SkyboxToneMapEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

SkyboxToneMapEffect& SkyboxToneMapEffect::operator=(SkyboxToneMapEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

SkyboxToneMapEffect& SkyboxToneMapEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("BasicEffect needs an instance!");
    return *g_pInstance;
}

bool SkyboxToneMapEffect::InitAll(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // ******************
    // 创建顶点着色器
    //

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SkyboxToneMapVS", L"Shaders\\SkyboxToneMap.hlsl",
        device, "SkyboxToneMapVS", "vs_5_0"));

    // ******************
    // 创建像素着色器
    //

    D3D_SHADER_MACRO defines[2] = {};
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SkyboxToneMap_Cube_Reinhard_PS", L"Shaders\\SkyboxToneMap.hlsl",
        device, "SkyboxToneMapCubePS", "ps_5_0"));
    defines[0].Name = "TONEMAP_STANDARD";
    defines[0].Definition = "";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SkyboxToneMap_Cube_Standard_PS", L"Shaders\\SkyboxToneMap.hlsl",
        device, "SkyboxToneMapCubePS", "ps_5_0", defines));
    defines[0].Name = "TONEMAP_ACES_COARSE";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SkyboxToneMap_Cube_AcesCoarse_PS", L"Shaders\\SkyboxToneMap.hlsl",
        device, "SkyboxToneMapCubePS", "ps_5_0", defines));
    defines[0].Name = "TONEMAP_ACES";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SkyboxToneMap_Cube_Aces_PS", L"Shaders\\SkyboxToneMap.hlsl",
        device, "SkyboxToneMapCubePS", "ps_5_0", defines));



    // ******************
    // 创建Pass
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "SkyboxToneMapVS";
    passDesc.namePS = "SkyboxToneMap_Cube_Reinhard_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SkyboxToneMap_Cube_Reinhard", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_Reinhard");
        pPass->SetRasterizerState(RenderStates::RSNoCull.Get());
    }
    passDesc.namePS = "SkyboxToneMap_Cube_Standard_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SkyboxToneMap_Cube_Standard", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_Standard");
        pPass->SetRasterizerState(RenderStates::RSNoCull.Get());
    }
    passDesc.namePS = "SkyboxToneMap_Cube_AcesCoarse_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SkyboxToneMap_Cube_ACES_Coarse", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_ACES_Coarse");
        pPass->SetRasterizerState(RenderStates::RSNoCull.Get());
    }
    passDesc.namePS = "SkyboxToneMap_Cube_Aces_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SkyboxToneMap_Cube_ACES", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_ACES");
        pPass->SetRasterizerState(RenderStates::RSNoCull.Get());
    }

    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearWrap", RenderStates::SSLinearWrap.Get());

    // 设置调试对象名
    pImpl->m_pEffectHelper->SetDebugObjectName("SkyboxToneMapEffect");

    return true;
}

void XM_CALLCONV SkyboxToneMapEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    UNREFERENCED_PARAMETER(W);
}

void XM_CALLCONV SkyboxToneMapEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV SkyboxToneMapEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void SkyboxToneMapEffect::RenderCubeSkyboxWithToneMap(
    ID3D11DeviceContext* deviceContext,
    ID3D11ShaderResourceView* cubeTexture, 
    ID3D11ShaderResourceView* depthTexture, 
    ID3D11ShaderResourceView* litTexture, 
    ID3D11RenderTargetView* outputTexture,
    const D3D11_VIEWPORT& vp,
    ToneMapping tm)
{
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &outputTexture, nullptr);

    pImpl->m_pEffectHelper->SetShaderResourceByName("g_SkyboxTexture", cubeTexture);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DepthTexture", depthTexture);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_LitTexture", litTexture);
    XMMATRIX VP = XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
    VP = XMMatrixTranspose(VP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&VP);
    
    switch (tm)
    {
    case SkyboxToneMapEffect::ToneMapping_Standard:
        pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_Standard")->Apply(deviceContext);
        break;
    case SkyboxToneMapEffect::ToneMapping_Reinhard:
        pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_Reinhard")->Apply(deviceContext);
        break;
    case SkyboxToneMapEffect::ToneMapping_ACES_Coarse:
        pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_ACES_Coarse")->Apply(deviceContext);
        break;
    case SkyboxToneMapEffect::ToneMapping_ACES:
        pImpl->m_pEffectHelper->GetEffectPass("SkyboxToneMap_Cube_ACES")->Apply(deviceContext);
        break;
    default:
        break;
    }

    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->Draw(36, 0);
    
    // 清空
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    ID3D11ShaderResourceView* nullSRVs[3]{};
    deviceContext->PSSetShaderResources(0, 3, nullSRVs);
}

void SkyboxToneMapEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    UNREFERENCED_PARAMETER(deviceContext);
}

