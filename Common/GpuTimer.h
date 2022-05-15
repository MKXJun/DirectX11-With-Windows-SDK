//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 获取GPU两个时间戳的间隔
// Retrieve the interval between two timestamps of the GPU.
//***************************************************************************************

#pragma once

#ifndef GPU_TIMER_H
#define GPU_TIMER_H

#include <cassert>
#include <cstdint>
#include <deque>
#include <wrl/client.h>
#include <d3d11_1.h>

struct GpuTimerInfo
{
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData {};
    uint64_t startData = 0;
    uint64_t stopData = 0;
    Microsoft::WRL::ComPtr<ID3D11Query> disjointQuery;
    Microsoft::WRL::ComPtr<ID3D11Query> startQuery;
    Microsoft::WRL::ComPtr<ID3D11Query> stopQuery;
    bool isStopped = false;
};

class GpuTimer
{
public:
    GpuTimer() = default;
    
    // recentCount为0时统计所有间隔的平均值
    // 否则统计最近N帧间隔的平均值
    void Init(ID3D11Device* device, size_t recentCount = 0);
    
    // 重置平均用时
    // recentCount为0时统计所有间隔的平均值
    // 否则统计最近N帧间隔的平均值
    void Reset(size_t recentCount = 0);
    // 给命令队列插入起始时间戳
    HRESULT Start();
    // 给命令队列插入结束时间戳
    void Stop();
    // 尝试获取间隔
    bool TryGetTime(double* pOut);
    // 强制获取间隔
    double GetTime();
    // 计算平均用时
    double AverageTime()
    {
        if (m_RecentCount)
            return m_AccumTime / m_DeltaTimes.size();
        else
            return m_AccumTime / m_AccumCount;
    }

private:
    
    static bool GetQueryDataHelper(ID3D11DeviceContext* pContext, bool loopUntilDone, ID3D11Query* query, void* data, uint32_t dataSize);
    

    std::deque<double> m_DeltaTimes;
    double m_AccumTime = 0.0;
    size_t m_AccumCount = 0;
    size_t m_RecentCount = 0;
    size_t m_FrameID = 0;

    std::deque<GpuTimerInfo> m_Queries;
    std::deque<Microsoft::WRL::ComPtr<ID3D11Query>> m_pEndTimeQuery;
    Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pImmediateContext;
};

#endif // GAMETIMER_H