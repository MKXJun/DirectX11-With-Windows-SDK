#include "Waves.h"
#include <ModelManager.h>
#include <TextureManager.h>
#include <DXTrace.h>

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;

uint32_t Waves::RowCount() const
{
    return m_NumRows;
}

uint32_t Waves::ColumnCount() const
{
    return m_NumCols;
}

void Waves::InitResource(ID3D11Device* device, uint32_t rows, uint32_t cols,
    float texU, float texV, float timeStep, float spatialStep,
    float waveSpeed, float damping, float flowSpeedX, float flowSpeedY, bool cpuWrite)
{
    m_NumRows = rows;
    m_NumCols = cols;

    m_TexU = texU;
    m_TexV = texV;

    m_TimeStep = timeStep;
    m_SpatialStep = spatialStep;
    m_FlowSpeedX = flowSpeedX;
    m_FlowSpeedY = flowSpeedY;
    m_AccumulateTime = 0.0f;

    float d = damping * timeStep + 2.0f;
    float e = (waveSpeed * waveSpeed) * (timeStep * timeStep) / (spatialStep * spatialStep);
    m_K1 = (damping * timeStep - 2.0f) / d;
    m_K2 = (4.0f - 8.0f * e) / d;
    m_K3 = (2.0f * e) / d;

    m_MeshData = Geometry::CreateGrid(XMFLOAT2((cols - 1) * spatialStep, (rows - 1) * spatialStep), XMUINT2(cols - 1, rows - 1), XMFLOAT2(1.0f, 1.0f));
    Model::CreateFromGeometry(m_Model, device, m_MeshData, cpuWrite);
    m_pModel = &m_Model;

    if (TextureManager::Get().GetTexture("..\\Texture\\water2.dds") == nullptr)
        TextureManager::Get().CreateFromFile("..\\Texture\\water2.dds", false, true);
    m_Model.materials[0].Set<std::string>("$Diffuse", "..\\Texture\\water2.dds");
    m_Model.materials[0].Set<XMFLOAT4>("$AmbientColor", XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));
    m_Model.materials[0].Set<XMFLOAT4>("$DiffuseColor", XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f));
    m_Model.materials[0].Set<XMFLOAT4>("$SpecularColor", XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
    m_Model.materials[0].Set<float>("$Opacity", 0.5f);
    m_Model.materials[0].Set<float>("$SpecularPower", 32.0f);
    m_Model.materials[0].Set<XMFLOAT2>("$TexOffset", XMFLOAT2());
    m_Model.materials[0].Set<XMFLOAT2>("$TexScale", XMFLOAT2(m_TexU, m_TexV));
}

void CpuWaves::InitResource(ID3D11Device* device,
    uint32_t rows, uint32_t cols, float texU, float texV, float timeStep, float spatialStep,
    float waveSpeed, float damping, float flowSpeedX, float flowSpeedY)
{
    Waves::InitResource(device, rows, cols, texU, texV, timeStep, 
        spatialStep, waveSpeed, damping, flowSpeedX, flowSpeedY, true);

    // 取出顶点数据
    m_CurrSolution.swap(m_MeshData.vertices);
    // 保持与顶点数目一致
    m_PrevSolution.resize(m_CurrSolution.size());
    m_CurrNormals.resize(m_CurrSolution.size());
    // 顶点位置复制到Prev
    std::copy(m_CurrSolution.begin(), m_CurrSolution.end(), m_PrevSolution.begin());
}

void CpuWaves::Update(float dt)
{
    m_AccumulateTime += dt;
    
    auto& texOffset = m_Model.materials[0].Get<XMFLOAT2>("$TexOffset");
    texOffset.x += m_FlowSpeedX * dt;
    texOffset.y += m_FlowSpeedY * dt;

    // 仅仅在累积时间大于时间步长时才更新
    if (m_AccumulateTime > m_TimeStep)
    {
        m_isUpdated = true;
        // 仅仅对内部顶点进行更新
        for (size_t i = 1; i < m_NumRows - 1; ++i)
        {
            for (size_t j = 1; j < m_NumCols - 1; ++j)
            {
                // 在这次更新之后，我们将丢弃掉上一次模拟的数据。
                // 因此我们将运算的结果保存到Prev[i][j]的位置上。
                // 注意我们能够使用这种原址更新是因为Prev[i][j]
                // 的数据仅在当前计算Next[i][j]的时候才用到
                m_PrevSolution[i * m_NumCols + j].y =
                    m_K1 * m_PrevSolution[i * m_NumCols + j].y +
                    m_K2 * m_CurrSolution[i * m_NumCols + j].y +
                    m_K3 * (m_CurrSolution[(i + 1) * m_NumCols + j].y +
                        m_CurrSolution[(i - 1) * m_NumCols + j].y +
                        m_CurrSolution[i * m_NumCols + j + 1].y +
                        m_CurrSolution[i * m_NumCols + j - 1].y);
            }
        }

        // 由于把下一次模拟的结果写到了上一次模拟的缓冲区内，
        // 我们需要将下一次模拟的结果与当前模拟的结果交换
        m_PrevSolution.swap(m_CurrSolution);

        m_AccumulateTime = 0.0f;    // 重置时间

        // 使用有限差分法计算法向量
        for (size_t i = 1; i < m_NumRows - 1; ++i)
        {
            for (size_t j = 1; j < m_NumCols - 1; ++j)
            {
                float left = m_CurrSolution[i * m_NumCols + j - 1].y;
                float right = m_CurrSolution[i * m_NumCols + j + 1].y;
                float top = m_CurrSolution[(i - 1) * m_NumCols + j].y;
                float bottom = m_CurrSolution[(i + 1) * m_NumCols + j].y;
                m_CurrNormals[i * m_NumCols + j] = XMFLOAT3(-right + left, 2.0f * m_SpatialStep, bottom - top);
                XMVECTOR nVec = XMVector3Normalize(XMLoadFloat3(&m_CurrNormals[i * m_NumCols + j]));
                XMStoreFloat3(&m_CurrNormals[i * m_NumCols + j], nVec);
            }
        }
    }
}

void CpuWaves::Disturb(uint32_t i, uint32_t j, float magnitude)
{
    // 不要对边界处激起波浪
    assert(i > 1 && i < m_NumRows - 2);
    assert(j > 1 && j < m_NumCols - 2);

    float halfMag = 0.5f * magnitude;

    // 对顶点[i][j]及其相邻顶点修改高度值
    size_t curr = i * (size_t)m_NumCols + j;
    m_CurrSolution[curr].y += magnitude;
    m_CurrSolution[curr - 1].y += halfMag;
    m_CurrSolution[curr + 1].y += halfMag;
    m_CurrSolution[curr - m_NumCols].y += halfMag;
    m_CurrSolution[curr + m_NumCols].y += halfMag;

    m_isUpdated = true;
}

void CpuWaves::Draw(ID3D11DeviceContext* deviceContext, IEffect& effect)
{
    // 更新动态顶点缓冲区的数据
    if (m_isUpdated)
    {
        m_isUpdated = false;
        // 更新数据
        D3D11_MAPPED_SUBRESOURCE mappedData;
        deviceContext->Map(m_Model.meshdatas[0].m_pVertices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
        memcpy_s(mappedData.pData, m_Model.meshdatas[0].m_VertexCount * sizeof(XMFLOAT3),
            m_CurrSolution.data(), (uint32_t)m_CurrSolution.size() * sizeof(XMFLOAT3));
        deviceContext->Unmap(m_Model.meshdatas[0].m_pVertices.Get(), 0);

        deviceContext->Map(m_Model.meshdatas[0].m_pNormals.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
        memcpy_s(mappedData.pData, m_Model.meshdatas[0].m_VertexCount * sizeof(XMFLOAT3),
            m_CurrNormals.data(), (uint32_t)m_CurrNormals.size() * sizeof(XMFLOAT3));
        deviceContext->Unmap(m_Model.meshdatas[0].m_pNormals.Get(), 0);
    }

    GameObject::Draw(deviceContext, effect);
}

std::unique_ptr<EffectHelper> GpuWaves::m_pEffectHelper = nullptr;

void GpuWaves::InitResource(ID3D11Device* device,
    uint32_t rows, uint32_t cols, float texU, float texV, 
    float timeStep, float spatialStep, float waveSpeed, float damping, float flowSpeedX, float flowSpeedY)
{
    // 初始化特效
    if (!m_pEffectHelper)
    {
        m_pEffectHelper = std::make_unique<EffectHelper>();
        ComPtr<ID3DBlob> blob;
        HR(D3DReadFileToBlob(L"Shaders/WavesDisturb_CS.cso", blob.ReleaseAndGetAddressOf()));
        HR(m_pEffectHelper->AddShader("WavesDisturb", device, blob.Get()));

        HR(D3DReadFileToBlob(L"Shaders/WavesUpdate_CS.cso", blob.ReleaseAndGetAddressOf()));
        HR(m_pEffectHelper->AddShader("WavesUpdate", device, blob.Get()));

        EffectPassDesc passDesc;
        passDesc.nameCS = "WavesDisturb";
        HR(m_pEffectHelper->AddEffectPass("WavesDisturb", device, &passDesc));
        passDesc.nameCS = "WavesUpdate";
        HR(m_pEffectHelper->AddEffectPass("WavesUpdate", device, &passDesc));
    }

    Waves::InitResource(device, rows, cols, texU, texV, timeStep, 
        spatialStep, waveSpeed, damping, flowSpeedX, flowSpeedY, false);


    m_pPrevSolutionTexture = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pCurrSolutionTexture = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
    m_pNextSolutionTexture = std::make_unique<Texture2D>(device, cols, rows, DXGI_FORMAT_R32_FLOAT, 1,
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
}

void GpuWaves::Update(ID3D11DeviceContext* deviceContext, float dt)
{
    m_AccumulateTime += dt;
    auto& texOffset = m_Model.materials[0].Get<XMFLOAT2>("$TexOffset");
    texOffset.x += m_FlowSpeedX * dt;
    texOffset.y += m_FlowSpeedY * dt;

    // 仅仅在累积时间大于时间步长时才更新
    if (m_AccumulateTime > m_TimeStep)
    {
        m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant0")->SetFloat(m_K1);
        m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant1")->SetFloat(m_K2);
        m_pEffectHelper->GetConstantBufferVariable("g_WaveConstant2")->SetFloat(m_K3);

        m_pEffectHelper->SetUnorderedAccessByName("g_PrevSolInput", m_pPrevSolutionTexture->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessByName("g_CurrSolInput", m_pCurrSolutionTexture->GetUnorderedAccess(), 0);
        m_pEffectHelper->SetUnorderedAccessByName("g_Output", m_pNextSolutionTexture->GetUnorderedAccess(), 0);
        auto pPass = m_pEffectHelper->GetEffectPass("WavesUpdate");
        pPass->Apply(deviceContext);
        pPass->Dispatch(deviceContext, m_NumCols, m_NumRows);

        // 清除绑定
        ID3D11UnorderedAccessView* nullUAVs[3]{};
        deviceContext->CSSetUnorderedAccessViews(0, 3, nullUAVs, nullptr);

        //
        // 对缓冲区进行Ping-pong交换以准备下一次更新
        // 上一次模拟的缓冲区不再需要，用作下一次模拟的输出缓冲
        // 当前模拟的缓冲区变成上一次模拟的缓冲区
        // 下一次模拟的缓冲区变换当前模拟的缓冲区
        //
        m_pCurrSolutionTexture.swap(m_pNextSolutionTexture);
        m_pPrevSolutionTexture.swap(m_pNextSolutionTexture);

        m_AccumulateTime = 0.0f;        // 重置时间
    }
}

void GpuWaves::Disturb(ID3D11DeviceContext* deviceContext, uint32_t i, uint32_t j, float magnitude)
{
    // 更新常量缓冲区
    m_pEffectHelper->GetConstantBufferVariable("g_DisturbMagnitude")->SetFloat(magnitude);
    uint32_t idx[2] = { j, i };
    m_pEffectHelper->GetConstantBufferVariable("g_DisturbIndex")->SetUIntVector(2, idx);

    m_pEffectHelper->SetUnorderedAccessByName("g_Output", m_pCurrSolutionTexture->GetUnorderedAccess(), 0);
    m_pEffectHelper->GetEffectPass("WavesDisturb")->Apply(deviceContext);

    deviceContext->Dispatch(1, 1, 1);

    // 清除绑定
    ID3D11UnorderedAccessView* nullUAVs[]{ nullptr };
    deviceContext->CSSetUnorderedAccessViews(m_pEffectHelper->MapUnorderedAccessSlot("g_Output"), 1, nullUAVs, nullptr);
}

void GpuWaves::Draw(ID3D11DeviceContext* deviceContext, BasicEffect& effect)
{
    // 设置位移贴图
    effect.SetTextureDisplacement(m_pCurrSolutionTexture->GetShaderResource());
    effect.SetWavesStates(true, m_SpatialStep);

    GameObject::Draw(deviceContext, effect);

    // 立即撤下位移贴图的绑定跟关闭水波绘制状态
    effect.SetTextureDisplacement(nullptr);
    effect.SetWavesStates(false);
    effect.Apply(deviceContext);
}


