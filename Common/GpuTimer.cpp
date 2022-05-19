#include "GpuTimer.h"

void GpuTimer::Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, size_t recentCount)
{
    m_pDevice = device;
    m_pImmediateContext = deviceContext;
    m_RecentCount = recentCount;
    m_AccumTime = 0.0;
    m_AccumCount = 0;
}

void GpuTimer::Reset(ID3D11DeviceContext* deviceContext, size_t recentCount)
{
    m_Queries.clear();
    m_DeltaTimes.clear();
    m_pImmediateContext = deviceContext;
    m_AccumTime = 0.0;
    m_AccumCount = 0;
    if (recentCount)
        m_RecentCount = recentCount;
}

HRESULT GpuTimer::Start()
{
    if (!m_Queries.empty() && !m_Queries.back().isStopped)
        return E_FAIL;

    // 创建查询
    GpuTimerInfo& info = m_Queries.emplace_back();
    CD3D11_QUERY_DESC queryDesc(D3D11_QUERY_TIMESTAMP);
    m_pDevice->CreateQuery(&queryDesc, info.startQuery.GetAddressOf());
    m_pDevice->CreateQuery(&queryDesc, info.stopQuery.GetAddressOf());
    queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    m_pDevice->CreateQuery(&queryDesc, info.disjointQuery.GetAddressOf());

    // 插入时间戳、开始连续性/频率查询
    m_pImmediateContext->Begin(info.disjointQuery.Get());
    m_pImmediateContext->End(info.startQuery.Get());
    return S_OK;
}

void GpuTimer::Stop()
{
    GpuTimerInfo& info = m_Queries.back();
    m_pImmediateContext->End(info.disjointQuery.Get());
    m_pImmediateContext->End(info.stopQuery.Get());
    info.isStopped = true;
}

bool GpuTimer::TryGetTime(double* pOut)
{
    if (m_Queries.empty())
        return false;

    GpuTimerInfo& info = m_Queries.front();
    if (!info.isStopped) return false;
    
    if (info.disjointQuery && !GetQueryDataHelper(m_pImmediateContext.Get(), false, info.disjointQuery.Get(), &info.disjointData, sizeof(info.disjointData)))
        return false;
    info.disjointQuery.Reset();

    if (info.startQuery && !GetQueryDataHelper(m_pImmediateContext.Get(), false, info.startQuery.Get(), &info.startData, sizeof(info.startData)))
        return false;
    info.startQuery.Reset();

    if (info.stopQuery && !GetQueryDataHelper(m_pImmediateContext.Get(), false, info.stopQuery.Get(), &info.stopData, sizeof(info.stopData)))
        return false;
    info.stopQuery.Reset();

    if (!info.disjointData.Disjoint)
    {
        double deltaTime = static_cast<double>(info.stopData - info.startData) / info.disjointData.Frequency;
        if (m_RecentCount > 0)
            m_DeltaTimes.push_back(deltaTime);
        m_AccumTime += deltaTime;
        m_AccumCount++;
        if (m_DeltaTimes.size() > m_RecentCount)
        {
            m_AccumTime -= m_DeltaTimes.front();
            m_DeltaTimes.pop_front();
        }
        if (pOut) *pOut = deltaTime;
    }
    else
    {
        double deltaTime = -1.0;
    }

    m_Queries.pop_front();
    return true;
}

double GpuTimer::GetTime()
{
    if (m_Queries.empty())
        return -1.0;

    GpuTimerInfo& info = m_Queries.front();
    if (!info.isStopped) return -1.0;

    if (info.disjointQuery)
    {
        GetQueryDataHelper(m_pImmediateContext.Get(), true, info.disjointQuery.Get(), &info.disjointData, sizeof(info.disjointData));
        info.disjointQuery.Reset();
    }
    if (info.startQuery)
    {
        GetQueryDataHelper(m_pImmediateContext.Get(), true, info.startQuery.Get(), &info.startData, sizeof(info.startData));
        info.startQuery.Reset();
    }
    if (info.stopQuery)
    {
        GetQueryDataHelper(m_pImmediateContext.Get(), true, info.stopQuery.Get(), &info.stopData, sizeof(info.stopData));
        info.stopQuery.Reset();
    }

    double deltaTime = -1.0;
    if (!info.disjointData.Disjoint)
    {
        deltaTime = static_cast<double>(info.stopData - info.startData) / info.disjointData.Frequency;
        if (m_RecentCount > 0)
            m_DeltaTimes.push_back(deltaTime);
        m_AccumTime += deltaTime;
        m_AccumCount++;
        if (m_DeltaTimes.size() > m_RecentCount)
        {
            m_AccumTime -= m_DeltaTimes.front();
            m_DeltaTimes.pop_front();
        }
    }

    m_Queries.pop_front();
    return deltaTime;
}

bool GpuTimer::GetQueryDataHelper(ID3D11DeviceContext* pContext, bool loopUntilDone, ID3D11Query* query, void* data, uint32_t dataSize)
{
    if (query == nullptr)
        return false;

    HRESULT hr = S_OK;
    int attempts = 0;
    do
    {
        // 尝试GPU回读
        hr = pContext->GetData(query, data, dataSize, 0);
        if (hr == S_OK)
            return true;
        attempts++;
        if (attempts > 100)
            Sleep(1);
        if (attempts > 1000)
        {
            assert(false);
            return false;
        }
    } while (loopUntilDone && (hr == S_FALSE));
    return false;
}