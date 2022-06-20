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
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;
    ComPtr<ID3D11InputLayout> m_pCurrInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
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

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");
    // ******************
    // 创建顶点着色器
    //

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowVS", L"Shaders\\Shadow.hlsl", device, 
        "ShadowVS", "vs_5_0", nullptr, blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("FullScreenTriangleTexcoordVS", L"Shaders\\Shadow.hlsl",
        device, "FullScreenTriangleTexcoordVS", "vs_5_0"));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowTessVS", L"Shaders\\Shadow.hlsl", device,
        "ShadowTessVS", "vs_5_0"));

    // ******************
    // 创建外壳着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowTessHS", L"Shaders\\Shadow.hlsl", device,
        "ShadowTessHS", "hs_5_0"));

    // ******************
    // 创建域着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowTessDS", L"Shaders\\Shadow.hlsl", device,
        "ShadowTessDS", "ds_5_0"));

    // ******************
    // 创建像素着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ShadowPS", L"Shaders\\Shadow.hlsl", device, "ShadowPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugShadowPS", L"Shaders\\Shadow.hlsl", device, "DebugShadowPS", "ps_5_0"));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "ShadowVS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DepthOnly", device, &passDesc));
    pImpl->m_pEffectHelper->GetEffectPass("DepthOnly")->SetRasterizerState(RenderStates::RSShadow.Get());

    passDesc.namePS = "ShadowPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DepthAlphaClip", device, &passDesc));
    pImpl->m_pEffectHelper->GetEffectPass("DepthAlphaClip")->SetRasterizerState(RenderStates::RSShadow.Get());

    passDesc.nameVS = "ShadowTessVS";
    passDesc.nameHS = "ShadowTessHS";
    passDesc.nameDS = "ShadowTessDS";
    passDesc.namePS = "";
    HR(pImpl->m_pEffectHelper->AddEffectPass("TessDepthOnly", device, &passDesc));
    pImpl->m_pEffectHelper->GetEffectPass("TessDepthOnly")->SetRasterizerState(RenderStates::RSShadow.Get());

    passDesc.namePS = "ShadowPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("TessDepthAlphaClip", device, &passDesc));
    pImpl->m_pEffectHelper->GetEffectPass("TessDepthAlphaClip")->SetRasterizerState(RenderStates::RSShadow.Get());

    passDesc.nameVS = "FullScreenTriangleTexcoordVS";
    passDesc.namePS = "DebugShadowPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugShadow", device, &passDesc));

    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());

    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "ShadowEffect.VertexPosNormalTexLayout");
#endif
    pImpl->m_pEffectHelper->SetDebugObjectName("ShadowEffect");

    return true;
}

void ShadowEffect::SetRenderDepthOnly(bool enableAlphaClip)
{
    if (enableAlphaClip)
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DepthAlphaClip");
    else
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DepthOnly");
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout;
    pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void ShadowEffect::SetRenderDepthOnlyWithDisplacement(bool enableAlphaClip)
{
    if (enableAlphaClip)
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("TessDepthAlphaClip");
    else
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("TessDepthOnly");
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout;
    pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
}

void ShadowEffect::RenderDepthToTexture(
    ID3D11DeviceContext* deviceContext,
    ID3D11ShaderResourceView* input,
    ID3D11RenderTargetView* output,
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DebugShadow");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", input);
    pImpl->m_pCurrEffectPass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->Draw(3, 0);

    int slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_DiffuseMap");
    input = nullptr;
    deviceContext->PSSetShaderResources(slot, 1, &input);
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
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

void ShadowEffect::SetMaterial(const Material& material)
{
    TextureManager& tm = TextureManager::Get();

    auto pStr = material.TryGet<std::string>("$Diffuse");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", pStr ? tm.GetTexture(*pStr) : nullptr);
    pStr = material.TryGet<std::string>("$Normal");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", pStr ? tm.GetTexture(*pStr) : nullptr);
}

MeshDataInput ShadowEffect::GetInputData(const MeshData& meshData)
{
    MeshDataInput input;
    input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
    input.topology = pImpl->m_CurrTopology;
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

void ShadowEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (FLOAT*)&eyePos);
}

void ShadowEffect::SetHeightScale(float scale)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_HeightScale")->SetFloat(scale);
}

void ShadowEffect::SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessDistance")->SetFloat(maxTessDistance);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessDistance")->SetFloat(minTessDistance);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinTessFactor")->SetFloat(minTessFactor);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxTessFactor")->SetFloat(maxTessFactor);
}


void ShadowEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
    XMMATRIX VP = XMLoadFloat4x4(&pImpl->m_View) * XMLoadFloat4x4(&pImpl->m_Proj);
    XMMATRIX WVP = W * VP;
    XMMATRIX WINVT = XMath::InverseTranspose(W);

    W = XMMatrixTranspose(W);
    VP = XMMatrixTranspose(VP);
    WVP = XMMatrixTranspose(WVP);
    WINVT = XMMatrixTranspose(WINVT);

    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_World")->SetFloatMatrix(4, 4, (const FLOAT*)&W);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&VP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTranspose")->SetFloatMatrix(4, 4, (const FLOAT*)&WINVT);

    pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

