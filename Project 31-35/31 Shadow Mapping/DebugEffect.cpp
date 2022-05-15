#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)

//
// DebugEffect::Impl 需要先于ShadowEffect的定义
//

class DebugEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;
    
    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// DebugEffect
//

namespace
{
    // DebugEffect单例
    static DebugEffect* g_pInstance = nullptr;
}

DebugEffect::DebugEffect()
{
    if (g_pInstance)
        throw std::exception("DebugEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<DebugEffect::Impl>();
}

DebugEffect::~DebugEffect()
{
}

DebugEffect::DebugEffect(DebugEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

DebugEffect& DebugEffect::operator=(DebugEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

DebugEffect& DebugEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("DebugEffect needs an instance!");
    return *g_pInstance;
}

bool DebugEffect::InitAll(ID3D11Device* device)
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

    HR(CreateShaderFromFile(L"HLSL\\DebugTexture_VS.cso", L"HLSL\\DebugTexture_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("DebugTexture_VS", device, blob.Get()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    // ******************
    // 创建像素着色器
    //

    HR(CreateShaderFromFile(L"HLSL\\DebugTextureRGBA_PS.cso", L"HLSL\\DebugTextureRGBA_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("DebugTextureRGBA_PS", device, blob.Get()));

    HR(CreateShaderFromFile(L"HLSL\\DebugTextureOneComp_PS.cso", L"HLSL\\DebugTextureOneComp_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("DebugTextureOneComp_PS", device, blob.Get()));

    HR(CreateShaderFromFile(L"HLSL\\DebugTextureOneCompGray_PS.cso", L"HLSL\\DebugTextureOneCompGray_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("DebugTextureOneCompGray_PS", device, blob.Get()));


    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "DebugTexture_VS";
    passDesc.namePS = "DebugTextureRGBA_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugTextureRGBA", device, &passDesc));
    passDesc.nameVS = "DebugTexture_VS";
    passDesc.namePS = "DebugTextureOneComp_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugTextureOneComp", device, &passDesc));
    passDesc.nameVS = "DebugTexture_VS";
    passDesc.namePS = "DebugTextureOneCompGray_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugTextureOneCompGray", device, &passDesc));

    // 设置采样器
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

    // 设置调试对象名
    D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "DebugEffect.VertexPosNormalTexLayout");
    pImpl->m_pEffectHelper->SetDebugObjectName("DebugEffect");

    return true;
}

void DebugEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DebugTextureRGBA");
}

void DebugEffect::SetRenderOneComponent(ID3D11DeviceContext* deviceContext, int index)
{
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DebugTextureOneComp");
    pImpl->m_pCurrEffectPass->PSGetParamByName("index")->SetSInt(index);
}

void DebugEffect::SetRenderOneComponentGray(ID3D11DeviceContext* deviceContext, int index)
{
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DebugTextureOneCompGray");
    pImpl->m_pCurrEffectPass->PSGetParamByName("index")->SetSInt(index);
}

void XM_CALLCONV DebugEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV DebugEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV DebugEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void DebugEffect::SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", textureDiffuse);
}

void DebugEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX WVP = XMLoadFloat4x4(&pImpl->m_World) * XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
    WVP = XMMatrixTranspose(WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
    pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

