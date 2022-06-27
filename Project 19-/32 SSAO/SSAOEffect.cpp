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
// SSAOEffect::Impl 需要先于BasicEffect的定义
//

# pragma warning(disable: 26812)

class SSAOEffect::Impl
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

    ComPtr<ID3D11SamplerState> m_pSamNormalDepth;
    ComPtr<ID3D11SamplerState> m_pSamRandomVec;
    ComPtr<ID3D11SamplerState> m_pSamBlur;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
};

//
// SSAOEffect
//

namespace
{
    // SSAOEffect单例
    static SSAOEffect* g_pInstance = nullptr;
}

SSAOEffect::SSAOEffect()
{
    if (g_pInstance)
        throw std::exception("SSAOEffect is a singleton!");
    g_pInstance = this;
    pImpl = std::make_unique<SSAOEffect::Impl>();
}

SSAOEffect::~SSAOEffect()
{
}

SSAOEffect::SSAOEffect(SSAOEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
}

SSAOEffect& SSAOEffect::operator=(SSAOEffect&& moveFrom) noexcept
{
    pImpl.swap(moveFrom.pImpl);
    return *this;
}

SSAOEffect& SSAOEffect::Get()
{
    if (!g_pInstance)
        throw std::exception("SSAOEffect needs an instance!");
    return *g_pInstance;
}


bool SSAOEffect::InitAll(ID3D11Device* device)
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

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_GeometryVS", L"Shaders\\SSAO.hlsl",
        device, "GeometryVS", "vs_5_0", nullptr, blob.ReleaseAndGetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::GetInputLayout(), ARRAYSIZE(VertexPosNormalTex::GetInputLayout()),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_VS", L"Shaders\\SSAO.hlsl",
        device, "SSAO_VS", "vs_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile(
        "FullScreenTriangleTexcoordVS", L"Shaders\\SSAO.hlsl",
        device, "FullScreenTriangleTexcoordVS", "vs_5_0"));

    // ******************
    // 创建像素着色器
    //
    D3D_SHADER_MACRO defines[]{
        { "BLUR_HORZ", "" },
        { nullptr, nullptr }
    };
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_GeometryPS", L"Shaders\\SSAO.hlsl", device, "GeometryPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_PS", L"Shaders\\SSAO.hlsl", device, "SSAO_PS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_BilateralVertPS", L"Shaders\\SSAO.hlsl", device, "BilateralPS", "ps_5_0"));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("SSAO_BilateralHorzPS", L"Shaders\\SSAO.hlsl", device, "BilateralPS", "ps_5_0", defines));
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile("DebugAO_PS", L"Shaders\\SSAO.hlsl", device, "DebugAO_PS", "ps_5_0"));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "SSAO_GeometryVS";
    passDesc.namePS = "SSAO_GeometryPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_Geometry", device, &passDesc));
    passDesc.nameVS = "SSAO_VS";
    passDesc.namePS = "SSAO_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO", device, &passDesc));
    passDesc.nameVS = "FullScreenTriangleTexcoordVS";
    passDesc.namePS = "SSAO_BilateralHorzPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_BlurHorz", device, &passDesc));
    passDesc.nameVS = "FullScreenTriangleTexcoordVS";
    passDesc.namePS = "SSAO_BilateralVertPS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_BlurVert", device, &passDesc));
    passDesc.nameVS = "FullScreenTriangleTexcoordVS";
    passDesc.namePS = "DebugAO_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("DebugAO", device, &passDesc));

    // ******************
    // 创建和设置采样器
    //
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearWrap", RenderStates::SSLinearWrap.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearClamp", RenderStates::SSLinearClamp.Get());

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof samplerDesc);

    // 用于法向量和深度的采样器
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[3] = 1e5f;	// 设置非常大的深度值 (Normal, depthZ) = (0, 0, 0, 1e5f)
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HR(device->CreateSamplerState(&samplerDesc, pImpl->m_pSamNormalDepth.GetAddressOf()));
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamNormalDepth", pImpl->m_pSamNormalDepth.Get());

    // 用于随机向量的采样器
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.BorderColor[3] = 0.0f;
    HR(device->CreateSamplerState(&samplerDesc, pImpl->m_pSamRandomVec.GetAddressOf()));
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamRandomVec", pImpl->m_pSamRandomVec.Get());

    // 用于模糊的采样器
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    HR(device->CreateSamplerState(&samplerDesc, pImpl->m_pSamBlur.GetAddressOf()));
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamBlur", pImpl->m_pSamBlur.Get());


    // 设置调试对象名
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "SSAOEffect.VertexPosNormalTexLayout");
    SetDebugObjectName(pImpl->m_pSamNormalDepth.Get(), "SSAOEffect.SSNormalDepth");
    SetDebugObjectName(pImpl->m_pSamRandomVec.Get(), "SSAOEffect.SSRandomVec");
    SetDebugObjectName(pImpl->m_pSamBlur.Get(), "SSAOEffect.SSBlur");
#endif
    pImpl->m_pEffectHelper->SetDebugObjectName("SSAOEffect");

    return true;
}

void XM_CALLCONV SSAOEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
    XMStoreFloat4x4(&pImpl->m_World, W);
}

void XM_CALLCONV SSAOEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV SSAOEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void SSAOEffect::SetMaterial(const Material& material)
{
    TextureManager& tm = TextureManager::Get();

    auto pStr = material.TryGet<std::string>("$Diffuse");
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", pStr ? tm.GetTexture(*pStr) : tm.GetNullTexture());
}

MeshDataInput SSAOEffect::GetInputData(const MeshData& meshData)
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

void SSAOEffect::SetRenderNormalDepthMap(bool enableAlphaClip)
{
    pImpl->m_pCurrInputLayout = pImpl->m_pVertexPosNormalTexLayout.Get();
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_Geometry");
    pImpl->m_CurrTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pImpl->m_pCurrEffectPass->PSGetParamByName("alphaClip")->SetUInt(enableAlphaClip);
}

void SSAOEffect::SetTextureRandomVec(ID3D11ShaderResourceView* textureRandomVec)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_RandomVecMap", textureRandomVec);
}

void SSAOEffect::SetOffsetVectors(const DirectX::XMFLOAT4 offsetVectors[14])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OffsetVectors")->SetRaw(offsetVectors);
}

void SSAOEffect::RenderToSSAOTexture(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* normalDepth, 
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp, 
    uint32_t sampleCount)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalDepthMap", normalDepth);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO");
    pPass->PSGetParamByName("sampleCount")->SetUInt(sampleCount);
    pPass->Apply(deviceContext);
    deviceContext->Draw(3, 0);

    // 清空
    normalDepth = nullptr;
    output = nullptr;
    deviceContext->OMSetRenderTargets(0, &output, nullptr);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_NormalDepthMap"), 1, &normalDepth);

}

void SSAOEffect::SetOcclusionInfo(float radius, float fadeStart, float fadeEnd, float surfaceEpsilon)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionRadius")->SetFloat(radius);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionFadeStart")->SetFloat(fadeStart);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionFadeEnd")->SetFloat(fadeEnd);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SurfaceEpsilon")->SetFloat(surfaceEpsilon);
}

void SSAOEffect::SetFrustumFarPlanePoints(const DirectX::XMFLOAT4 farPlanePoints[3])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FarPlanePoints")->SetRaw(farPlanePoints);
}

void SSAOEffect::SetBlurWeights(const float weights[11])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurWeights")->SetRaw(weights);
}

void SSAOEffect::SetBlurRadius(int radius)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurRadius")->SetSInt(radius);
}

void SSAOEffect::BilateralBlurX(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input,
    ID3D11ShaderResourceView* normalDepth,
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_InputImage", input);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalDepthMap", normalDepth);
    float texelSize[2] = { 1.0f / vp.Width, 1.0f / vp.Height };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexelSize")->SetFloatVector(2, texelSize);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_BlurHorz");
    pPass->Apply(deviceContext);
    deviceContext->Draw(3, 0);

    // 清空
    input = nullptr;
    output = nullptr;
    normalDepth = nullptr;
    deviceContext->OMSetRenderTargets(0, &output, nullptr);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_InputImage"), 1, &input);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_NormalDepthMap"), 1, &normalDepth);
}

void SSAOEffect::BilateralBlurY(
    ID3D11DeviceContext* deviceContext,
    ID3D11ShaderResourceView* input,
    ID3D11ShaderResourceView* normalDepth,
    ID3D11RenderTargetView* output,
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->RSSetViewports(1, &vp);
    deviceContext->OMSetRenderTargets(1, &output, nullptr);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_InputImage", input);
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalDepthMap", normalDepth);
    float texelSize[2] = { 1.0f / vp.Width, 1.0f / vp.Height };
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TexelSize")->SetFloatVector(2, texelSize);
    auto pPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_BlurVert");
    pPass->Apply(deviceContext);
    deviceContext->Draw(3, 0);

    // 清空
    input = nullptr;
    output = nullptr;
    normalDepth = nullptr;
    deviceContext->OMSetRenderTargets(0, &output, nullptr);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_InputImage"), 1, &input);
    deviceContext->PSSetShaderResources(pImpl->m_pEffectHelper->MapShaderResourceSlot("g_NormalDepthMap"), 1, &normalDepth);
}

void SSAOEffect::RenderAmbientOcclusionToTexture(
    ID3D11DeviceContext* deviceContext, 
    ID3D11ShaderResourceView* input, 
    ID3D11RenderTargetView* output, 
    const D3D11_VIEWPORT& vp)
{
    deviceContext->IASetInputLayout(nullptr);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("DebugAO");
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

void SSAOEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    XMMATRIX WV = W * V;
    XMMATRIX WVP = WV * P;
    XMMATRIX WInvTV = XMath::InverseTranspose(W) * V;

    WV = XMMatrixTranspose(WV);
    WInvTV = XMMatrixTranspose(WInvTV);
    WVP = XMMatrixTranspose(WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (const FLOAT*)&WV);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTransposeView")->SetFloatMatrix(4, 4, (const FLOAT*)&WInvTV);

    // 从NDC空间[-1, 1]^2变换到纹理空间[0, 1]^2
    static const XMMATRIX T = XMMATRIX(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);
    // 从观察空间到纹理空间的变换
    XMMATRIX PT = P * T;

    PT = XMMatrixTranspose(PT);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewToTexSpace")->SetFloatMatrix(4, 4, (const FLOAT*)&PT);

    pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

