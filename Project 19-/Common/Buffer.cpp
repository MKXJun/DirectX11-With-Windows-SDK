#include "Buffer.h"
#include "XUtil.h"

Buffer::Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc)
    : m_ByteWidth(bufferDesc.ByteWidth)
{
    d3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pBuffer.GetAddressOf());

    if (bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), nullptr, m_pUnorderedAccess.GetAddressOf());
    }

    if (bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), nullptr, m_pShaderResource.GetAddressOf());
    }
}

Buffer::Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc, 
    const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc,
    const CD3D11_UNORDERED_ACCESS_VIEW_DESC& uavDesc)
    : m_ByteWidth(bufferDesc.ByteWidth)
{
    d3dDevice->CreateBuffer(&bufferDesc, nullptr, m_pBuffer.GetAddressOf());

    if (bufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), &uavDesc, m_pUnorderedAccess.GetAddressOf());
        
    }

    if (bufferDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
        d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), &srvDesc, m_pShaderResource.GetAddressOf());
    }
}

void* Buffer::MapDiscard(ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return mappedResource.pData;
}

void Buffer::Unmap(ID3D11DeviceContext* d3dDeviceContext)
{
    d3dDeviceContext->Unmap(m_pBuffer.Get(), 0);
}

void Buffer::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG))

    ::SetDebugObjectName(m_pBuffer.Get(), name);
    if (m_pShaderResource)
        ::SetDebugObjectName(m_pShaderResource.Get(), std::string(name) + ".SRV");
    if (m_pUnorderedAccess)
        ::SetDebugObjectName(m_pUnorderedAccess.Get(), std::string(name) + ".UAV");

#else
    UNREFERENCED_PARAMETER(name);
#endif
}

ByteAddressBuffer::ByteAddressBuffer(ID3D11Device* d3dDevice, uint32_t numUInt32s, uint32_t bindFlags, bool dynamic, bool indirectArgs)
    : Buffer(d3dDevice, 
        CD3D11_BUFFER_DESC(
            numUInt32s * 4, bindFlags,
            dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
            dynamic ? D3D11_CPU_ACCESS_WRITE : 0,
            D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS | (indirectArgs ? D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS : 0)),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(
            m_pBuffer.Get(), DXGI_FORMAT_R32_TYPELESS, 0, numUInt32s, D3D11_BUFFEREX_SRV_FLAG_RAW),
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(
            m_pBuffer.Get(), DXGI_FORMAT_R32_TYPELESS, 0, numUInt32s, D3D11_BUFFER_UAV_FLAG_RAW)),
    m_NumUInt32s(numUInt32s)
{
}
