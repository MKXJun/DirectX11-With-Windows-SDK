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
#include "WinMin.h"
#include <d3d11_1.h>

struct GpuTimerInfo
{
    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData{}; // 频率/连续性信息
    uint64_t startData = 0;                             // 起始时间戳
    uint64_t stopData = 0;                              // 结束时间戳
    Microsoft::WRL::ComPtr<ID3D11Query> disjointQuery;  // 连续性查询
    Microsoft::WRL::ComPtr<ID3D11Query> startQuery;     // 起始时间戳查询
    Microsoft::WRL::ComPtr<ID3D11Query> stopQuery;      // 结束时间戳查询
    bool isStopped = false;                             // 是否插入了结束时间戳
};

class GpuTimer
{
public:
    GpuTimer() = default;
    
    // recentCount为0时统计所有间隔的平均值
    // 否则统计最近N帧间隔的平均值
    void Init(ID3D11Device* device, ID3D11DeviceContext* deviceContext, size_t recentCount = 0);
    
    // 重置平均用时
    // recentCount为0时统计所有间隔的平均值
    // 否则统计最近N帧间隔的平均值
    void Reset(ID3D11DeviceContext* deviceContext, size_t recentCount = 0);
    // 给命令队列插入起始时间戳
    HRESULT Start();
    // 给命令队列插入结束时间戳
    void Stop();
    // 尝试获取间隔
    bool TryGetTime(double* pOut);
    // 强制获取间隔(可能会造成阻塞)
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
    

    std::deque<double> m_DeltaTimes;    // 最近N帧的查询间隔
    double m_AccumTime = 0.0;           // 查询间隔的累计总和
    size_t m_AccumCount = 0;            // 完成回读的查询次数
    size_t m_RecentCount = 0;           // 保留最近N帧，0则包含所有

    std::deque<GpuTimerInfo> m_Queries; // 缓存未完成的查询
    Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pImmediateContext;
};

#endif // GAMETIMER_H