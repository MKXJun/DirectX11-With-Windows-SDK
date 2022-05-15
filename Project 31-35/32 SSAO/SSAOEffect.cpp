#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;

# pragma warning(disable: 26812)

//
// SSAOEffect::Impl 需要先于SSAOEffect的定义
//

class SSAOEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;

    ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;
    ComPtr<ID3D11InputLayout> m_pInstancePosNormalTexLayout;

    ComPtr<ID3D11SamplerState> m_pSamNormalDepth;
    ComPtr<ID3D11SamplerState> m_pSamRandomVec;
    ComPtr<ID3D11SamplerState> m_pSamBlur;

    XMFLOAT4X4 m_World{}, m_View{}, m_Proj{};
    RenderType m_RenderType = RenderObject;
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

    ComPtr<ID3DBlob> blob;

    // 实例输入布局
    D3D11_INPUT_ELEMENT_DESC instLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "World", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "World", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "World", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "World", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "WorldInvTranspose", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "WorldInvTranspose", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 80, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "WorldInvTranspose", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 96, D3D11_INPUT_PER_INSTANCE_DATA, 1},
        { "WorldInvTranspose", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 112, D3D11_INPUT_PER_INSTANCE_DATA, 1}
    };

    // ******************
    // 创建顶点着色器
    //

    HR(CreateShaderFromFile(L"HLSL\\SSAO_NormalDepth_Object_VS.cso", L"HLSL\\SSAO_NormalDepth_Object_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_NormalDepth_Object_VS", device, blob.Get()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\SSAO_NormalDepth_Instance_VS.cso", L"HLSL\\SSAO_NormalDepth_Instance_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_NormalDepth_Instance_VS", device, blob.Get()));
    // 创建顶点布局
    HR(device->CreateInputLayout(instLayout, ARRAYSIZE(instLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pInstancePosNormalTexLayout.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\SSAO_VS.cso", L"HLSL\\SSAO_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_VS", device, blob.Get()));

    HR(CreateShaderFromFile(L"HLSL\\SSAO_Blur_VS.cso", L"HLSL\\SSAO_Blur_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_Blur_VS", device, blob.Get()));

    // ******************
    // 创建像素着色器
    //

    HR(CreateShaderFromFile(L"HLSL\\SSAO_NormalDepth_PS.cso", L"HLSL\\SSAO_NormalDepth_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_NormalDepth_PS", device, blob.Get()));

    HR(CreateShaderFromFile(L"HLSL\\SSAO_PS.cso", L"HLSL\\SSAO_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_PS", device, blob.Get()));

    HR(CreateShaderFromFile(L"HLSL\\SSAO_Blur_PS.cso", L"HLSL\\SSAO_Blur_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SSAO_Blur_PS", device, blob.Get()));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "SSAO_NormalDepth_Object_VS";
    passDesc.namePS = "SSAO_NormalDepth_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_NormalDepth_Object", device, &passDesc));
    passDesc.nameVS = "SSAO_NormalDepth_Instance_VS";
    passDesc.namePS = "SSAO_NormalDepth_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_NormalDepth_Instance", device, &passDesc));
    passDesc.nameVS = "SSAO_VS";
    passDesc.namePS = "SSAO_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO", device, &passDesc));
    passDesc.nameVS = "SSAO_Blur_VS";
    passDesc.namePS = "SSAO_Blur_PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("SSAO_Blur", device, &passDesc));

    // ******************
    // 创建和设置采样器
    //
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearWrap", RenderStates::SSLinearWrap.Get());
    
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
    D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalTexLayout.Get(), "SSAOEffect.VertexPosNormalTexLayout");
    D3D11SetDebugObjectName(pImpl->m_pInstancePosNormalTexLayout.Get(), "SSAOEffect.m_pInstancePosNormalTexLayout");
    D3D11SetDebugObjectName(pImpl->m_pSamNormalDepth.Get(), "SSAOEffect.SSNormalDepth");
    D3D11SetDebugObjectName(pImpl->m_pSamRandomVec.Get(), "SSAOEffect.SSRandomVec");
    D3D11SetDebugObjectName(pImpl->m_pSamBlur.Get(), "SSAOEffect.SSBlur");
    pImpl->m_pEffectHelper->SetDebugObjectName("SSAOEffect");

    return true;
}

void SSAOEffect::SetRenderNormalDepth(ID3D11DeviceContext* deviceContext, RenderType type, bool enableAlphaClip)
{
    if (type == RenderObject)
    {
        deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_NormalDepth_Object");
    }
    else
    {
        deviceContext->IASetInputLayout(pImpl->m_pInstancePosNormalTexLayout.Get());
        pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_NormalDepth_Instance");
    }
    pImpl->m_RenderType = type;
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImpl->m_pCurrEffectPass->PSGetParamByName("alphaClip")->SetUInt(enableAlphaClip);
    
}

void SSAOEffect::SetRenderSSAOMap(ID3D11DeviceContext* deviceContext, int sampleCount)
{
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO");
    pImpl->m_pCurrEffectPass->PSGetParamByName("sampleCount")->SetSInt(sampleCount);
}

void SSAOEffect::SetRenderBilateralBlur(ID3D11DeviceContext* deviceContext, bool horizontalBlur)
{
    deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("SSAO_Blur");
    pImpl->m_pCurrEffectPass->PSGetParamByName("horizontalBlur")->SetUInt(horizontalBlur);
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

void SSAOEffect::SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_DiffuseMap", textureDiffuse);
}

void SSAOEffect::SetTextureNormalDepth(ID3D11ShaderResourceView* textureNormalDepth)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_NormalDepthMap", textureNormalDepth);
}

void SSAOEffect::SetTextureRandomVec(ID3D11ShaderResourceView* textureRandomVec)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_RandomVecMap", textureRandomVec);
}

void SSAOEffect::SetTextureBlur(ID3D11ShaderResourceView* textureBlur)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_InputImage", textureBlur);
}

void SSAOEffect::SetOffsetVectors(const DirectX::XMFLOAT4 offsetVectors[14])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OffsetVectors")->SetRaw(offsetVectors);
}

void SSAOEffect::SetFrustumCorners(const DirectX::XMFLOAT4 frustumCorners[4])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_FrustumCorners")->SetRaw(frustumCorners);
}

void SSAOEffect::SetOcclusionInfo(float radius, float fadeStart, float fadeEnd, float surfaceEpsilon)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionRadius")->SetFloat(radius);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionFadeStart")->SetFloat(fadeStart);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_OcclusionFadeEnd")->SetFloat(fadeEnd);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SurfaceEpsilon")->SetFloat(surfaceEpsilon);
}

void SSAOEffect::SetBlurWeights(const float weights[11])
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurWeights")->SetRaw(weights);
}

void SSAOEffect::SetBlurRadius(int radius)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_BlurRadius")->SetSInt(radius);
}

void SSAOEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    if (pImpl->m_RenderType == RenderObject)
    {
        XMMATRIX W = XMLoadFloat4x4(&pImpl->m_World);
        XMMATRIX WV = W * V;
        XMMATRIX WVP = WV * P;
        XMMATRIX WInvT = InverseTranspose(W);
        XMMATRIX WInvTV = WInvT * V;

        WV = XMMatrixTranspose(WV);
        WInvTV = XMMatrixTranspose(WInvTV);
        WVP = XMMatrixTranspose(WVP);
        pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldView")->SetFloatMatrix(4, 4, (const FLOAT*)&WV);
        pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&WVP);
        pImpl->m_pEffectHelper->GetConstantBufferVariable("g_WorldInvTransposeView")->SetFloatMatrix(4, 4, (const FLOAT*)&WInvTV);
    }

    // 从NDC空间[-1, 1]^2变换到纹理空间[0, 1]^2
    static const XMMATRIX T = XMMATRIX(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);
    // 从观察空间到纹理空间的变换
    XMMATRIX PT = P * T;

    V = XMMatrixTranspose(V);
    P = XMMatrixTranspose(P);
    PT = XMMatrixTranspose(PT);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_View")->SetFloatMatrix(4, 4, (const FLOAT*)&V);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_Proj")->SetFloatMatrix(4, 4, (const FLOAT*)&P);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewToTexSpace")->SetFloatMatrix(4, 4, (const FLOAT*)&PT);

    pImpl->m_pCurrEffectPass->Apply(deviceContext);
}

