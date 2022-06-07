
#pragma once

#ifndef DEBUG_LAYER_H
#define DEBUG_LAYER_H

#include "WinMin.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <deque>
#include <string>


class DebugLayer
{
public:
    ~DebugLayer() { ClearMessages(); }

    HRESULT Init(ID3D11Device* device);

    // 报告存活对象
    void ReportLiveDeviceObjects(D3D11_RLDO_FLAGS detailLevel) { m_pDebug->ReportLiveDeviceObjects(detailLevel); }
    
    // 是否关掉将消息输出到调试输出窗口
    void MuteDebugOutput(bool mute) { m_pInfoQueue->SetMuteDebugOutput(mute); }

    // 将信息队列中的所有消息缓存，并清空ID3D11InfoQueue中的消息
    const std::vector<D3D11_MESSAGE*>& FetchMessages();
    
    // 清空缓存的所有消息
    void ClearMessages();
    
    
private:
    Microsoft::WRL::ComPtr<ID3D11Debug> m_pDebug;
    Microsoft::WRL::ComPtr<ID3D11InfoQueue> m_pInfoQueue;
    std::vector<D3D11_MESSAGE*> m_pMessages;
};





#endif