#include "ParticleRender.h"
#include "Vertex.h"
#include "d3dUtil.h"

float ParticleRender::GetAge() const
{
    return m_Age;
}

void ParticleRender::SetEmitPos(const DirectX::XMFLOAT3& emitPos)
{
    m_EmitPos = emitPos;
}

void ParticleRender::SetEmitDir(const DirectX::XMFLOAT3& emitDir)
{
    m_EmitDir = emitDir;
}

void ParticleRender::SetEmitInterval(float t)
{
    m_EmitInterval = t;
}

void ParticleRender::SetAliveTime(float t)
{
    m_AliveTime = t;
}

HRESULT ParticleRender::Init(ID3D11Device* device, UINT maxParticles)
{
    m_MaxParticles = maxParticles;

    // 创建缓冲区用于产生粒子系统
    // 初始粒子拥有类型0和存活时间0
    HRESULT hr;
    VertexParticle p;
    ZeroMemory(&p, sizeof(VertexParticle));
    p.age = 0.0f;
    p.type = 0;
    hr = CreateVertexBuffer(device, &p, sizeof(VertexParticle), m_pInitVB.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // 创建Ping-Pong的缓冲区用于流输出和绘制
    hr = CreateVertexBuffer(device, nullptr, sizeof(VertexParticle) * m_MaxParticles, m_pDrawVB.GetAddressOf(), false, true);
    if (FAILED(hr))
        return hr;

    hr = CreateVertexBuffer(device, nullptr, sizeof(VertexParticle) * m_MaxParticles, m_pStreamOutVB.GetAddressOf(), false, true);
    return hr;
}

void ParticleRender::SetTextureArraySRV(ID3D11ShaderResourceView* textureArraySRV)
{
    m_pTextureArraySRV = textureArraySRV;
}

void ParticleRender::SetRandomTexSRV(ID3D11ShaderResourceView* randomTexSRV)
{
    m_pRandomTexSRV = randomTexSRV;
}

void ParticleRender::Reset()
{
    m_FirstRun = true;
    m_Age = 0.0f;
}

void ParticleRender::Update(float dt, float gameTime)
{
    m_GameTime = gameTime;
    m_TimeStep = dt;

    m_Age += dt;
}

void ParticleRender::Draw(ID3D11DeviceContext* deviceContext, ParticleEffect& effect, const Camera& camera)
{
    effect.SetGameTime(m_GameTime);
    effect.SetTimeStep(m_TimeStep);
    effect.SetEmitPos(m_EmitPos);
    effect.SetEmitDir(m_EmitDir);
    effect.SetEmitInterval(m_EmitInterval);
    effect.SetAliveTime(m_AliveTime);
    effect.SetTextureArray(m_pTextureArraySRV.Get());
    effect.SetTextureRandom(m_pRandomTexSRV.Get());

    // ******************
    // 流输出
    //
    effect.SetRenderToVertexBuffer(deviceContext);
    UINT strides[1] = { sizeof(VertexParticle) };
    UINT offsets[1] = { 0 };

    // 如果是第一次运行，使用初始顶点缓冲区
    // 否则，使用存有当前所有粒子的顶点缓冲区
    if (m_FirstRun)
        deviceContext->IASetVertexBuffers(0, 1, m_pInitVB.GetAddressOf(), strides, offsets);
    else
        deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), strides, offsets);

    // 经过流输出写入到顶点缓冲区
    deviceContext->SOSetTargets(1, m_pStreamOutVB.GetAddressOf(), offsets);
    effect.Apply(deviceContext);
    if (m_FirstRun)
    {
        deviceContext->Draw(1, 0);
        m_FirstRun = false;
    }
    else
    {
        deviceContext->DrawAuto();
    }

    // 解除缓冲区绑定
    ID3D11Buffer* nullBuffers[1] = { nullptr };
    deviceContext->SOSetTargets(1, nullBuffers, offsets);

    // 进行顶点缓冲区的Ping-Pong交换
    m_pDrawVB.Swap(m_pStreamOutVB);

    // ******************
    // 使用流输出顶点绘制粒子
    //
    effect.SetRenderDefault(deviceContext);

    deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), strides, offsets);
    effect.Apply(deviceContext);
    deviceContext->DrawAuto();
}

void ParticleRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    D3D11SetDebugObjectName(m_pInitVB.Get(), name + ".InitVB");
    D3D11SetDebugObjectName(m_pStreamOutVB.Get(), name + ".StreamVB");
    D3D11SetDebugObjectName(m_pDrawVB.Get(), name + ".DrawVB");
    D3D11SetDebugObjectName(m_pRandomTexSRV.Get(), name + ".RandomTexSRV");
    D3D11SetDebugObjectName(m_pTextureArraySRV.Get(), name + ".TextureArraySRV");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
