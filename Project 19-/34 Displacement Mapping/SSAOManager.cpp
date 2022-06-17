#include "SSAOManager.h"
#include <XUtil.h>
#include <DirectXPackedVector.h>
#include <random>

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;


void SSAOManager::InitResource(ID3D11Device* device, int width, int height)
{
    OnResize(device, width, height);
    BuildOffsetVectors();
    BuildRandomVectorTexture(device);
}

void SSAOManager::OnResize(ID3D11Device* device, int width, int height)
{
    m_pNormalDepthTexture = std::make_unique<Texture2D>(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_pAOTexture = std::make_unique<Texture2D>(device, width / 2, height / 2, DXGI_FORMAT_R16_FLOAT);
    m_pAOTempTexture = std::make_unique<Texture2D>(device, width / 2, height / 2, DXGI_FORMAT_R16_FLOAT);

    m_pNormalDepthTexture->SetDebugObjectName("NormalDepthTexture");
    m_pAOTexture->SetDebugObjectName("SSAOTexture");
    m_pAOTempTexture->SetDebugObjectName("SSAOTempTexture");
}


void SSAOManager::Begin(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* dsv, const D3D11_VIEWPORT& vp)
{

    // 将指定的DSV绑定到管线上
    ID3D11RenderTargetView* pRTVs[1] = { m_pNormalDepthTexture->GetRenderTarget() };
    deviceContext->OMSetRenderTargets(1, pRTVs, dsv);
    deviceContext->RSSetViewports(1, &vp);

    // 使用观察空间法向量(0, 0, -1)和非常大的深度值来清空RTV
    static const float clearColor[4] = { 0.0f, 0.0f, -1.0f, 1e5f };
    deviceContext->ClearRenderTargetView(pRTVs[0], clearColor);
    deviceContext->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void SSAOManager::End(ID3D11DeviceContext* deviceContext)
{
    // 解除RTV、DSV
    deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void SSAOManager::RenderToSSAOTexture(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, const Camera& camera)
{
    float zFar = camera.GetFarZ();
    float halfHeight = zFar * tanf(0.5f * camera.GetFovY());
    float halfWidth = camera.GetAspectRatio() * halfHeight;

    // 单个三角形渲染，提供的远平面点满足
    // (-1, 1)--(3, 1)
    //   |     /
    //   |    /
    //   |   /       
    // (-1, -3)
    XMFLOAT4 farPlanePoints[3] = {
        XMFLOAT4(-halfWidth, halfHeight, zFar, 0.0f),
        XMFLOAT4(3.0f * halfWidth, halfHeight, zFar, 0.0f),
        XMFLOAT4(-halfWidth, -3.0f * halfHeight, zFar, 0.0f),
    };
    ssaoEffect.SetFrustumFarPlanePoints(farPlanePoints);
    ssaoEffect.SetTextureRandomVec(m_pRandomVectorTexture->GetShaderResource());
    ssaoEffect.SetOffsetVectors(m_Offsets);
    ssaoEffect.SetProjMatrix(camera.GetProjMatrixXM());
    ssaoEffect.SetOcclusionInfo(m_OcclusionRadius, m_OcclusionFadeStart, m_OcclusionFadeEnd, m_SurfaceEpsilon);
    CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pAOTexture->GetWidth(), (float)m_pAOTexture->GetHeight());
    ssaoEffect.RenderToSSAOTexture(deviceContext, m_pNormalDepthTexture->GetShaderResource(), m_pAOTexture->GetRenderTarget(), vp, m_SampleCount);
}

void SSAOManager::BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect)
{
    CD3D11_VIEWPORT vp(0.0f, 0.0f, (float)m_pAOTempTexture->GetWidth(), (float)m_pAOTempTexture->GetHeight());
    ssaoEffect.SetBlurRadius(m_BlurRadius);
    ssaoEffect.SetBlurWeights(m_BlurWeights);
    // 每次模糊需要进行一次水平模糊和垂直模糊
    for (uint32_t i = 0; i < m_BlurCount; ++i)
    {
        ssaoEffect.BilateralBlurX(deviceContext, m_pAOTexture->GetShaderResource(), m_pNormalDepthTexture->GetShaderResource(), m_pAOTempTexture->GetRenderTarget(), vp);
        ssaoEffect.BilateralBlurY(deviceContext, m_pAOTempTexture->GetShaderResource(), m_pNormalDepthTexture->GetShaderResource(), m_pAOTexture->GetRenderTarget(), vp);
    }
}

ID3D11ShaderResourceView* SSAOManager::GetAmbientOcclusionTexture()
{
    return m_pAOTexture->GetShaderResource();
}

ID3D11ShaderResourceView* SSAOManager::GetNormalDepthTexture()
{
    return m_pNormalDepthTexture->GetShaderResource();
}

void SSAOManager::BuildOffsetVectors()
{
    // 从14个均匀分布的向量开始。我们选择立方体的8个角点，并沿着立方体的每个面选取中心点
    // 我们总是让这些点以相对另一边的形式交替出现。这种办法可以在我们选择少于14个采样点
    // 时仍然能够让向量均匀散开

    // 8个立方体角点向量
    m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

    m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

    m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

    m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

    // 6个面中心点向量
    m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
    m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

    m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
    m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

    m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
    m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);


    // 初始化随机数数据
    std::mt19937 randEngine;
    randEngine.seed(std::random_device()());
    std::uniform_real_distribution<float> randF(0.25f, 1.0f);
    for (int i = 0; i < 14; ++i)
    {
        // 创建长度范围在[0.25, 1.0]内的随机长度的向量
        float s = randF(randEngine);

        XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

        XMStoreFloat4(&m_Offsets[i], v);
    }
}

void SSAOManager::BuildRandomVectorTexture(ID3D11Device* device)
{
    m_pRandomVectorTexture = std::make_unique<Texture2D>(device, 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_pRandomVectorTexture->SetDebugObjectName("RandomVectorTexture");

    std::vector<XMCOLOR> randomVectors(256 * 256);

    ComPtr<ID3D11DeviceContext> pContext;
    device->GetImmediateContext(pContext.GetAddressOf());

    // 初始化随机数数据
    std::mt19937 randEngine;
    randEngine.seed(std::random_device()());
    std::uniform_real_distribution<float> randF(0.0f, 1.0f);
    for (int i = 0; i < 256 * 256; ++i)
    {
        randomVectors[i] = XMCOLOR(randF(randEngine), randF(randEngine), randF(randEngine), 0.0f);
    }
    pContext->UpdateSubresource(m_pRandomVectorTexture->GetTexture(), 0, nullptr, randomVectors.data(), 256 * sizeof(XMCOLOR), 0);
}

