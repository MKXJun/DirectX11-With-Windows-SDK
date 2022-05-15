#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"

using namespace DirectX;

# pragma warning(disable: 26812)

//
// ParticleEffect::Impl 需要先于BasicEffect的定义
//

class ParticleEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;

    ComPtr<ID3D11InputLayout> m_pVertexParticleLayout;
};

//
// ParticleEffect
//

ParticleEffect::ParticleEffect()
{
    pImpl = std::make_unique<ParticleEffect::Impl>();
}

ParticleEffect::~ParticleEffect()
{
}

ParticleEffect::ParticleEffect(ParticleEffect&& moveFrom) noexcept
{
    this->pImpl.swap(moveFrom.pImpl);
}

ParticleEffect& ParticleEffect::operator=(ParticleEffect&& moveFrom) noexcept
{
    this->pImpl.swap(moveFrom.pImpl);
    return *this;
}

bool ParticleEffect::Init(ID3D11Device* device, const std::wstring& effectPath)
{
    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    const D3D11_SO_DECLARATION_ENTRY particleLayout[5] = {
        {0, "POSITION", 0, 0, 3, 0},
        {0, "VELOCITY", 0, 0, 3, 0},
        {0, "SIZE", 0, 0, 2, 0},
        {0, "AGE", 0, 0, 1, 0},
        {0, "TYPE", 0, 0, 1, 0}
    };

    ComPtr<ID3DBlob> blob;

    // ******************
    // 创建顶点着色器
    //
    std::wstring sovsHlsl = effectPath + L"_SO_VS.hlsl";
    std::wstring sovsCso = effectPath + L"_SO_VS.cso";
    HR(CreateShaderFromFile(sovsCso.c_str(), sovsHlsl.c_str(), "VS", "vs_5_0", blob.GetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("SO_VS", device, blob.Get()));
    // 创建顶点布局
    HR(device->CreateInputLayout(VertexParticle::inputLayout, ARRAYSIZE(VertexParticle::inputLayout),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexParticleLayout.GetAddressOf()));
    

    std::wstring vsHlsl = effectPath + L"_VS.hlsl";
    std::wstring vsCso = effectPath + L"_VS.cso";
    HR(CreateShaderFromFile(vsCso.c_str(), vsHlsl.c_str(), "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("VS", device, blob.Get()));
    // ******************
    // 创建几何着色器
    //

    // 流输出几何着色器
    std::wstring sogsHlsl = effectPath + L"_SO_GS.hlsl";
    std::wstring sogsCso = effectPath + L"_SO_GS.cso";
    HR(CreateShaderFromFile(sogsCso.c_str(), sogsHlsl.c_str(), "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));

    ComPtr<ID3D11GeometryShader> pGS;
    UINT strides[1] = { sizeof(VertexParticle) };
    HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(),
        particleLayout, ARRAYSIZE(particleLayout), strides, 1, D3D11_SO_NO_RASTERIZED_STREAM,
        nullptr, pGS.GetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddGeometryShaderWithStreamOutput("SO_GS", device, pGS.Get(), blob.Get()));

    // 几何着色器
    std::wstring gsHlsl = effectPath + L"_GS.hlsl";
    std::wstring gsCso = effectPath + L"_GS.cso";
    HR(CreateShaderFromFile(gsCso.c_str(), gsHlsl.c_str(), "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("GS", device, blob.Get()));

    // ******************
    // 创建像素着色器
    //
    std::wstring psHlsl = effectPath + L"_PS.hlsl";
    std::wstring psCso = effectPath + L"_PS.cso";
    HR(CreateShaderFromFile(psCso.c_str(), psHlsl.c_str(), "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddShader("PS", device, blob.Get()));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    passDesc.nameVS = "SO_VS";
    passDesc.nameGS = "SO_GS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("StreamOutput", device, &passDesc));
    pImpl->m_pEffectHelper->GetEffectPass("StreamOutput")->SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);
    passDesc.nameVS = "VS";
    passDesc.nameGS = "GS";
    passDesc.namePS = "PS";
    HR(pImpl->m_pEffectHelper->AddEffectPass("Render", device, &passDesc));

    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinear", RenderStates::SSLinearWrap.Get());

    return true;
}

void ParticleEffect::SetRenderToVertexBuffer(ID3D11DeviceContext* deviceContext)
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("StreamOutput");
    deviceContext->IASetInputLayout(pImpl->m_pVertexParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void ParticleEffect::SetRenderDefault(ID3D11DeviceContext* deviceContext)
{
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Render");
    deviceContext->IASetInputLayout(pImpl->m_pVertexParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

void XM_CALLCONV ParticleEffect::SetViewProjMatrix(DirectX::FXMMATRIX VP)
{
    XMMATRIX VPT = XMMatrixTranspose(VP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (const FLOAT*)&VPT);
}

void ParticleEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (const FLOAT*)&eyePos);
}

void ParticleEffect::SetGameTime(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_GameTime")->SetFloat(t);
}

void ParticleEffect::SetTimeStep(float step)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TimeStep")->SetFloat(step);
}

void ParticleEffect::SetEmitDir(const DirectX::XMFLOAT3& dir)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitDirW")->SetFloatVector(3, (const FLOAT*)&dir);
}

void ParticleEffect::SetEmitPos(const DirectX::XMFLOAT3& pos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitPosW")->SetFloatVector(3, (const FLOAT*)&pos);
}

void ParticleEffect::SetEmitInterval(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitInterval")->SetFloat(t);
}

void ParticleEffect::SetAliveTime(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AliveTime")->SetFloat(t);
}

void ParticleEffect::SetTextureArray(ID3D11ShaderResourceView* textureArray)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TexArray", textureArray);
}

void ParticleEffect::SetTextureRandom(ID3D11ShaderResourceView* textureRandom)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_RandomTex", textureRandom);
}

void ParticleEffect::SetBlendState(ID3D11BlendState* blendState, const FLOAT blendFactor[4], UINT sampleMask)
{
    pImpl->m_pEffectHelper->GetEffectPass("Render")->SetBlendState(blendState, blendFactor, sampleMask);
}

void ParticleEffect::SetDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef)
{
    pImpl->m_pEffectHelper->GetEffectPass("Render")->SetDepthStencilState(depthStencilState, stencilRef);
}

void ParticleEffect::SetDebugObjectName(const std::string& name)
{
    // 设置调试对象名
    std::string layout = name + ".VertexParticleLayout";
    D3D11SetDebugObjectName(pImpl->m_pVertexParticleLayout.Get(), layout.c_str());
    pImpl->m_pEffectHelper->SetDebugObjectName(name);
}

void ParticleEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    if (pImpl->m_pCurrEffectPass)
        pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
