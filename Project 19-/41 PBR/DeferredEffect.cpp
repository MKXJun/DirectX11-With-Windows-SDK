#include "Effects.h"

#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>
using namespace DirectX;

#pragma warning(disable: 26812)


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
    ComPtr<ID3D11InputLayout> m_pCurrInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY m_Topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    
    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;
    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTangentTexLayout;
    
    bool m_AlphaClip = false;
    bool m_HasTangent = false;
    bool m_UseNormalMap = false;
    bool m_HasRoughnessMetallicMap = false;

    int m_PCFKernelSize = 1;
    uint32_t m_ShadowSize = 1024;
    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// DeferredEffect
//

namespace
{
    // DeferredEffect单例
    static DeferredEffect* g_pInstance = nullptr;
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

DeferredEffect::DeferredEffect(DeferredEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

DeferredEffect& DeferredEffect::operator=(DeferredEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

DeferredEffect& DeferredEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("DeferredEffect needs an instance!");
    return *g_pInstance;
}


bool DeferredEffect::InitAll(ID3D11Device* device)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(L"Shaders\\Cache");

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    D3D_SHADER_MACRO defines[4]{};

    // ******************
    // 创建顶点着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("FullScreenTriangleVS", L"Shaders\\FullScreenTriangle.hlsl",
        device, "FullScreenTriangleVS", "vs_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryVS", L"Shaders\\GBuffer.hlsl",
        device, "GeometryVS", "vs_5_0", nullptr, &blob));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), &pImpl->m_pVertexPosNormalTexLayout));

    defines[0].Name = "USE_NORMAL_MAP";
    defines[0].Definition = "";
    defines[1].Name = "USE_TANGENT";
    defines[1].Definition = "";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GeometryTangentVS", L"Shaders\\GBuffer.hlsl",
        device, "GeometryVS", "vs_5_0", defines, &blob));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTangentTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTangentTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), &pImpl->m_pVertexPosNormalTangentTexLayout));

    // ******************
    // 创建像素/计算着色器
    //

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0"));

    defines[1].Name = nullptr;
    defines[1].Definition = nullptr;
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferNormalMapPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0", defines));

    defines[1].Name = "USE_TANGENT";
    defines[1].Definition = "";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferNormalMapTangentPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0", defines));

    defines[0].Name = "USE_MIXED_ARM_MAP";
    defines[1].Name = nullptr;
    defines[1].Definition = nullptr;
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferMixedArmPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0", defines));

    defines[1].Name = "USE_NORMAL_MAP";
    defines[1].Definition = "";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferMixedArmNormalMapPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0", defines));

    defines[2].Name = "USE_TANGENT";
    defines[2].Definition = "";
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("GBufferMixedArmNormalMapTangentPS", L"Shaders\\GBuffer.hlsl",
        device, "GBufferPS", "ps_5_0", defines));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugNormalPS", L"Shaders\\GBuffer.hlsl",
        device, "DebugNormalPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugAlbedoPS", L"Shaders\\GBuffer.hlsl",
        device, "DebugAlbedoPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugMetallicPS", L"Shaders\\GBuffer.hlsl",
        device, "DebugMetallicPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugRoughnessPS", L"Shaders\\GBuffer.hlsl",
        device, "DebugRoughnessPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("ComputeShaderTileDeferredCS", L"Shaders\\ComputeShaderTile.hlsl",
        device, "ComputeShaderTileDeferredCS", "cs_5_0"));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "GeometryVS";
    passDesc.namePS = "GBufferPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBuffer", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBuffer");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.namePS = "GBufferNormalMapPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBufferNormalMap", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBufferNormalMap");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.nameVS = "GeometryTangentVS";
    passDesc.namePS = "GBufferNormalMapTangentPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBufferNormalMapTangent", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBufferNormalMapTangent");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.nameVS = "GeometryVS";
    passDesc.namePS = "GBufferMixedArmPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBufferMixedArm", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBufferMixedArm");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.namePS = "GBufferMixedArmNormalMapPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBufferMixedArmNormalMap", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBufferMixedArmNormalMap");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.nameVS = "GeometryTangentVS";
    passDesc.namePS = "GBufferMixedArmNormalMapTangentPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("GBufferMixedArmNormalMapTangent", device, &passDesc));
    {
        auto pPass = pImpl->m_pEffectHelper->GetEffectPass("GBufferMixedArmNormalMapTangent");
        // 注意：反向Z => GREATER_EQUAL测试
        pPass->SetDepthStencilState(RenderStates::DSSGreaterEqual.Get(), 0);
    }

    passDesc.nameVS = "FullScreenTriangleVS";
    passDesc.namePS = "DebugNormalPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugNormal", device, &passDesc));

    passDesc.namePS = "DebugAlbedoPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugAlbedo", device, &passDesc));

    passDesc.namePS = "DebugMetallicPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugMetallic", device, &passDesc));

    passDesc.namePS = "DebugRoughnessPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugRoughness", device, &passDesc));

    passDesc.nameVS = "";
    passDesc.namePS = "";
    passDesc.nameCS = "ComputeShaderTileDeferredCS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("ComputeShaderTileDeferred", device, &passDesc));

    pImpl->m_pEffectHelper->SetSamplerStateByName("g_Sam", RenderStates::SSLinearWrap.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamplerShadowCmp", RenderStates::SSShadowPCF.Get());

    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTangentTexLayout.Get(), "DeferredEffect.VertexPosNormalTangentTexLayout");
    SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "DeferredEffect.VertexPosNormalTexLayout");
    pImpl->m_pEffectHelper->SetDebugObjectName("DeferredEffect");
#endif

    return true;
}

void DeferredEffect::SetCascadeVisulization(bool enable)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_VisualizeCascades")->SetSInt(enable);
}

void DeferredEffect::SetCascadeOffsets(const DirectX::XMFLOAT4 offsets[4])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeOffset")->SetRaw(offsets);
}

void DeferredEffect::SetCascadeScales(const DirectX::XMFLOAT4 scales[4])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeScale")->SetRaw(scales);
}

void DeferredEffect::SetCascadeFrustumsEyeSpaceDepths(const float depths[4])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeFrustumsEyeSpaceDepthsData")->SetRaw(depths);
}

void DeferredEffect::SetCascadeBlendArea(float blendArea)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CascadeBlendArea")->SetFloat(blendArea);
}

void DeferredEffect::SetShadowSize(int size)
{
    pImpl->m_ShadowSize = size;
    float padding = 1.0f / (float)size;
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexelSize")->SetFloat(1.0f / size);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinBorderPadding")->SetFloat(padding);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxBorderPadding")->SetFloat(1.0f - padding);
}

void XM_CALLCONV DeferredEffect::SetShadowViewMatrix(DirectX::FXMMATRIX ShadowView)
{
    XMMATRIX ShadowViewT = XMMatrixTranspose(ShadowView);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ShadowView")->SetFloatMatrix(4, 4, (const float*)&ShadowViewT);
}

void DeferredEffect::SetPCFKernelSize(int size)
{
    int start = -size / 2;
    int end = size + start;
    pImpl->m_PCFKernelSize = size;
    float padding = (float)(size / 2) / (float)pImpl->m_ShadowSize;
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopStart")->SetSInt(start);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFBlurForLoopEnd")->SetSInt(end);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MinBorderPadding")->SetFloat(padding);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_MaxBorderPadding")->SetFloat(1.0f - padding);
}

void DeferredEffect::SetPCFDepthBias(float offset)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_PCFDepthBias")->SetFloat(offset);
}

void DeferredEffect::SetCameraNearFar(float nearZ, float farZ)
{
    float nearFar[4] = { nearZ, farZ };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_CameraNearFar")->SetFloatVector(4, nearFar);
}

void DeferredEffect::SetDirectionalLight(
    const DirectX::XMFLOAT3& lightDir,
    const DirectX::XMFLOAT3& lightColor,
    float lightIntensity)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLightDir")->SetFloatVector(3, (FLOAT*)&lightDir);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLightColor")->SetFloatVector(3, (FLOAT*)&lightColor);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DirLightIntensity")->SetFloat(lightIntensity);
}

void DeferredEffect::SetRenderGBuffer(bool alphaClip)
{
    pImpl->m_HasTangent = false;
    pImpl->m_UseNormalMap = false;
    pImpl->m_AlphaClip = alphaClip;
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout.Get();
    pImpl->m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

void DeferredEffect::SetRenderGBufferWithNormalMap(bool hasTangent, bool alphaClip)
{
    pImpl->m_HasTangent = hasTangent;
    pImpl->m_UseNormalMap = true;
    pImpl->m_AlphaClip = alphaClip;
    pImpl->m_pCurrInputLayout = hasTangent ? pImpl->m_pVertexPosNormalTangentTexLayout.Get() : pImpl->m_pVertexPosNormalTexLayout.Get();
    pImpl->m_Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
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

    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[0]", normalGBuffer);
    std::string passStr = "DebugNormal";
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passStr);
    pPass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
    deviceContext->Draw(3, 0);

    // 清空
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[0]", nullptr);
    pPass->Apply(deviceContext);
}

void DeferredEffect::DebugAlbedoGBuffer(ID3D11DeviceContext* deviceContext,
    ID3D11RenderTargetView* rtv,
    ID3D11ShaderResourceView* albedoGBuffer,
    D3D11_VIEWPORT viewport)
{
    // 设置全屏三角形
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    deviceContext->RSSetViewports(1, &viewport);

    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[1]", albedoGBuffer);
    std::string passStr = "DebugAlbedo";
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passStr);
    pPass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
    deviceContext->Draw(3, 0);

    // 清空
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[1]", nullptr);
    pPass->Apply(deviceContext);
}

void DeferredEffect::DebugRoughnessGBuffer(ID3D11DeviceContext* deviceContext, 
    ID3D11RenderTargetView* rtv, 
    ID3D11ShaderResourceView* roughnessGBuffer, 
    D3D11_VIEWPORT viewport)
{
    // 设置全屏三角形
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    deviceContext->RSSetViewports(1, &viewport);

    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[0]", roughnessGBuffer);
    std::string passStr = "DebugRoughness";
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passStr);
    pPass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
    deviceContext->Draw(3, 0);

    // 清空
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[0]", nullptr);
    pPass->Apply(deviceContext);
}

void DeferredEffect::DebugMetallicGBuffer(ID3D11DeviceContext* deviceContext,
    ID3D11RenderTargetView* rtv,
    ID3D11ShaderResourceView* metallicGBuffer,
    D3D11_VIEWPORT viewport)
{
    // 设置全屏三角形
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    deviceContext->RSSetViewports(1, &viewport);

    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[1]", metallicGBuffer);
    std::string passStr = "DebugMetallic";
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passStr);
    pPass->Apply(deviceContext);
    deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
    deviceContext->Draw(3, 0);

    // 清空
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[1]", nullptr);
    pPass->Apply(deviceContext);
}

void DeferredEffect::ComputeTiledLightCulling(ID3D11DeviceContext* deviceContext,
    ID3D11UnorderedAccessView* litBufferUAV,
    ID3D11ShaderResourceView* lightBufferSRV,
    ID3D11ShaderResourceView* shadowTextureArraySRV,
    ID3D11ShaderResourceView* GBuffers[3])
{
    // 不需要清空操作，我们写入所有的像素

    Microsoft::WRL::ComPtr<ID3D11Texture2D> pTex;
    GBuffers[0]->GetResource(reinterpret_cast<ID3D11Resource**>(pTex.GetAddressOf()));
    D3D11_TEXTURE2D_DESC texDesc;
    pTex->GetDesc(&texDesc);

    UINT dims[2] = { texDesc.Width, texDesc.Height };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FramebufferDimensions")->SetUIntVector(2, dims);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[0]", GBuffers[0]);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[1]", GBuffers[1]);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_GBufferTextures[2]", GBuffers[2]);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_Light", lightBufferSRV);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureShadow", shadowTextureArraySRV);
    pImpl->m_pEffectHelper->SetUnorderedAccessByName("g_FrameBuffer", litBufferUAV, 0);

    std::string passName = "ComputeShaderTileDeferred";
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
    pImpl->m_pCurrEffectPass->Apply(deviceContext);
    pImpl->m_pCurrEffectPass->Dispatch(deviceContext, texDesc.Width, texDesc.Height);

    // 清空

    int slot = pImpl->m_pEffectHelper->MapUnorderedAccessSlot("g_FrameBuffer");
    litBufferUAV = nullptr;
    deviceContext->CSSetUnorderedAccessViews(slot, 1, &litBufferUAV, nullptr);

    slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_Light");
    lightBufferSRV = nullptr;
    deviceContext->CSSetShaderResources(slot, 1, &lightBufferSRV);

    ID3D11ShaderResourceView* nullSRVs[3] = {};
    slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_GBufferTextures[0]");
    deviceContext->CSSetShaderResources(slot, 3, nullSRVs);

    slot = pImpl->m_pEffectHelper->MapShaderResourceSlot("g_TextureShadow");
    deviceContext->CSSetShaderResources(slot, 1, nullSRVs);
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

void DeferredEffect::SetMaterial(const Material& material)
{
    TextureManager& tm = TextureManager::Get();
    static const XMFLOAT4 max_albedo = { 0.8f, 0.8f, 0.8f, 1.0f };

    auto pStr = material.TryGet<std::string>("$Albedo");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_AlbedoMap", pStr ? tm.GetTexture(*pStr) : tm.GetTexture("$Null"));
    auto pVec4 = material.TryGet<XMFLOAT4>("$Albedo");
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_kAlbedo")->SetFloatVector(4, reinterpret_cast<const float*>(pVec4 ? pVec4 : &max_albedo));

    if (pImpl->m_UseNormalMap)
    {
        pStr = material.TryGet<std::string>("$Normal");
        pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalMap", pStr ? tm.GetTexture(*pStr) : tm.GetTexture("$Normal"));
    }
    
    auto pStrR = material.TryGet<std::string>("$Roughness");
    auto pStrM = material.TryGet<std::string>("$Metallic");
    if (pStrR && pStrM)
    {
        if (*pStrR == *pStrM)
        {
            pImpl->m_HasRoughnessMetallicMap = true;
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_RoughnessMetallicMap", tm.GetTexture(*pStrR));
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_RoughnessMap", nullptr);
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_MetallicMap", nullptr);
        }
        else
        {
            pImpl->m_HasRoughnessMetallicMap = false;
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_RoughnessMetallicMap", nullptr);
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_RoughnessMap", pStrR ? tm.GetTexture(*pStrR) : tm.GetTexture("$Ones"));
            pImpl->m_pEffectHelper->SetShaderResourceByName("g_MetallicMap", pStrM ? tm.GetTexture(*pStrM) : tm.GetTexture("$Zeros"));
        }
    }
    else
    {
        pImpl->m_pEffectHelper->SetShaderResourceByName("g_RoughnessMetallicMap", tm.GetTexture("$Ones"));
    }

    auto pValue = material.TryGet<float>("$Roughness");
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_kRoughness")->SetFloat(pValue ? *pValue : 1.0f);
    pValue = material.TryGet<float>("$Metallic");
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_kMetallic")->SetFloat(pValue ? *pValue : 1.0f);
}

MeshDataInput DeferredEffect::GetInputData(const MeshData& meshData)
{
    MeshDataInput input;
    input.pInputLayout = pImpl->m_pCurrInputLayout.Get();
    input.topology = pImpl->m_Topology;

    input.pVertexBuffers.reserve(4);
    input.strides.reserve(4);
    input.offsets.reserve(4);

    input.pVertexBuffers = {
        meshData.m_pVertices.Get(),
        meshData.m_pNormals.Get()
    };
    input.strides = { 12, 12 };
    input.offsets = { 0, 0 };

    if (pImpl->m_HasTangent)
    {
        input.pVertexBuffers.push_back(meshData.m_pTangents.Get());
        input.strides.push_back(16);
        input.offsets.push_back(0);
    }

    input.pVertexBuffers.push_back(meshData.m_pTexcoordArrays.empty() ? nullptr : meshData.m_pTexcoordArrays[0].Get());
    input.strides.push_back(8);
    input.offsets.push_back(0);

    input.pIndexBuffer = meshData.m_pIndices.Get();
    input.indexCount = meshData.m_IndexCount;

    return input;
}

void DeferredEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    XMMATRIX WV = W * V;
    XMMATRIX WVP = WV * P;
    XMMATRIX InvV = XMMatrixInverse(nullptr, V);
    XMMATRIX WInvTV = XMath::InverseTranspose(W) * V;

    WV = XMMatrixTranspose(WV);
    WVP = XMMatrixTranspose(WVP);
    InvV = XMMatrixTranspose(InvV);
    WInvTV = XMMatrixTranspose(WInvTV);
    P = XMMatrixTranspose(P);

    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTransposeView")->SetFloatMatrix(4, 4, (FLOAT*)&WInvTV);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (FLOAT*)&WV);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_InvView")->SetFloatMatrix(4, 4, (FLOAT*)&InvV);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (FLOAT*)&P);

    std::string passName;
    passName.reserve(40);
    passName = "GBuffer";
    if (pImpl->m_HasRoughnessMetallicMap)
        passName += "MixedArm";
    if (pImpl->m_UseNormalMap)
    {
        passName += "NormalMap";
        if (pImpl->m_HasTangent)
            passName += "Tangent";
    }
    else if (pImpl->m_HasTangent)
        passName += "NormalMapTangent";
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass(passName);
    pPass->PSGetParamByName("alphaClip")->SetSInt(pImpl->m_AlphaClip);
    pPass->Apply(deviceContext);
}


