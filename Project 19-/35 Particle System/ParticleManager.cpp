#include "ParticleManager.h"
#include <Vertex.h>
#include <XUtil.h>
#include <DXTrace.h>

float ParticleManager::GetAge() const
{
    return m_Age;
}

void ParticleManager::SetEmitPos(const DirectX::XMFLOAT3& emitPos)
{
    m_EmitPos = emitPos;
}

void ParticleManager::SetEmitDir(const DirectX::XMFLOAT3& emitDir)
{
    m_EmitDir = emitDir;
}

void ParticleManager::SetEmitInterval(float t)
{
    m_EmitInterval = t;
}

void ParticleManager::SetAliveTime(float t)
{
    m_AliveTime = t;
}

void ParticleManager::SetAcceleration(const DirectX::XMFLOAT3& accel)
{
    m_Accel = accel;
}

void ParticleManager::InitResource(ID3D11Device* device, uint32_t maxParticles)
{
    m_MaxParticles = maxParticles;

    // 创建缓冲区用于产生粒子系统
    // 初始粒子拥有类型0和存活时间0
    ParticleEffect::VertexParticle p{};
    p.age = 0.0f;
    p.type = 0;
    CD3D11_BUFFER_DESC bufferDesc(sizeof(ParticleEffect::VertexParticle),
        D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &p;
    HR(device->CreateBuffer(&bufferDesc, &initData, m_pInitVB.GetAddressOf()));

    // 创建Ping-Pong的缓冲区用于流输出和绘制
    bufferDesc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
    bufferDesc.ByteWidth = sizeof(ParticleEffect::VertexParticle) * m_MaxParticles;
    HR(device->CreateBuffer(&bufferDesc, nullptr, m_pDrawVB.GetAddressOf()));

    HR(device->CreateBuffer(&bufferDesc, nullptr, m_pStreamOutVB.GetAddressOf()));
}

void ParticleManager::SetTextureInput(ID3D11ShaderResourceView* textureInput)
{
    m_pTextureInputSRV = textureInput;
}

void ParticleManager::SetTextureRandom(ID3D11ShaderResourceView* randomTexSRV)
{
    m_pTextureRanfomSRV = randomTexSRV;
}

void ParticleManager::Reset()
{
    m_FirstRun = true;
    m_Age = 0.0f;
}

void ParticleManager::Update(float dt, float gameTime)
{
    m_GameTime = gameTime;
    m_TimeStep = dt;

    m_Age += dt;
}

void ParticleManager::Draw(ID3D11DeviceContext* deviceContext, ParticleEffect& effect)
{
    effect.SetGameTime(m_GameTime);
    effect.SetTimeStep(m_TimeStep);
    effect.SetEmitPos(m_EmitPos);
    effect.SetEmitDir(m_EmitDir);
    effect.SetAcceleration(m_Accel);
    effect.SetEmitInterval(m_EmitInterval);
    effect.SetAliveTime(m_AliveTime);
    effect.SetTextureInput(m_pTextureInputSRV.Get());
    effect.SetTextureRandom(m_pTextureRanfomSRV.Get());

    // ******************
    // 流输出
    //
    // 如果是第一次运行，使用初始顶点缓冲区
    // 否则，使用存有当前所有粒子的顶点缓冲区
    effect.RenderToVertexBuffer(deviceContext,
        m_FirstRun ? m_pInitVB.Get() : m_pDrawVB.Get(),
        m_pStreamOutVB.Get(),
        m_FirstRun);
    // 后续转为DrawAuto
    m_FirstRun = 0;


    // 进行顶点缓冲区的Ping-Pong交换
    m_pDrawVB.Swap(m_pStreamOutVB);

    // ******************
    // 使用流输出顶点绘制粒子
    //
    auto inputData = effect.SetRenderDefault();
    deviceContext->IASetPrimitiveTopology(inputData.topology);
    deviceContext->IASetInputLayout(inputData.pInputLayout);
    deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), &inputData.stride, &inputData.offset);
    effect.Apply(deviceContext);
    deviceContext->DrawAuto();
}

void ParticleManager::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pInitVB.Get(), name + ".InitVB");
    ::SetDebugObjectName(m_pStreamOutVB.Get(), name + ".StreamVB");
    ::SetDebugObjectName(m_pDrawVB.Get(), name + ".DrawVB");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
