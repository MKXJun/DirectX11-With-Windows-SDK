#include "DebugLayer.h"

HRESULT DebugLayer::Init(ID3D11Device* device)
{
    if (!device)
        return E_INVALIDARG;
    HRESULT hr;
    hr = device->QueryInterface(m_pDebug.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    hr = device->QueryInterface(m_pInfoQueue.ReleaseAndGetAddressOf());
    return hr;
}

const std::vector<D3D11_MESSAGE*>& DebugLayer::FetchMessages()
{
    ClearMessages();
    uint64_t numAllowedMessages = m_pInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
    m_pMessages.reserve(numAllowedMessages);
    for (uint64_t i = 0; i < numAllowedMessages; ++i)
    {
        size_t messageSize = 0;
        if (SUCCEEDED(m_pInfoQueue->GetMessage(i, nullptr, &messageSize)))
        {
            m_pMessages.push_back(reinterpret_cast<D3D11_MESSAGE*>(malloc(messageSize)));
            m_pInfoQueue->GetMessage(i, m_pMessages.back(), &messageSize);
        }
    }
    
    m_pInfoQueue->ClearStoredMessages();
    return m_pMessages;
}

void DebugLayer::ClearMessages()
{
    for (D3D11_MESSAGE* pMessage : m_pMessages)
        free(pMessage);
    m_pMessages.clear();
}
