
#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include "WinMin.h"
#include "D3DFormat.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <string_view>

class Buffer
{
public:
    Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc);
    Buffer(ID3D11Device* d3dDevice, const CD3D11_BUFFER_DESC& bufferDesc, 
        const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc, 
        const CD3D11_UNORDERED_ACCESS_VIEW_DESC& uavDesc);
    ~Buffer() = default;

    // 不允许拷贝，允许移动
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) = default;
    Buffer& operator=(Buffer&&) = default;

    ID3D11Buffer* GetBuffer() { return m_pBuffer.Get(); }
    ID3D11UnorderedAccessView* GetUnorderedAccess() { return m_pUnorderedAccess.Get(); }
    ID3D11ShaderResourceView* GetShaderResource() { return m_pShaderResource.Get(); }

    // 仅支持动态缓冲区
    // TODO: Support NOOVERWRITE ring buffer?
    void* MapDiscard(ID3D11DeviceContext* d3dDeviceContext);
    void Unmap(ID3D11DeviceContext* d3dDeviceContext);
    uint32_t GetByteWidth() const { return m_ByteWidth; }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name);

protected:
    template<class Type>
    using ComPtr = Microsoft::WRL::ComPtr<Type>;

    ComPtr<ID3D11Buffer> m_pBuffer;
    ComPtr<ID3D11ShaderResourceView> m_pShaderResource;
    ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccess;
    uint32_t m_ByteWidth = 0;
};


// 注意：确保T与着色器中结构体的大小/布局相同
template<class T>
class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(ID3D11Device* d3dDevice, uint32_t elements,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool enableCounter = false,
        bool dynamic = false);
    ~StructuredBuffer() = default;

    // 不允许拷贝，允许移动
    StructuredBuffer(const StructuredBuffer&) = delete;
    StructuredBuffer& operator=(const StructuredBuffer&) = delete;
    StructuredBuffer(StructuredBuffer&&) = default;
    StructuredBuffer& operator=(StructuredBuffer&&) = default;

    // 仅支持动态缓冲区
    // TODO: Support NOOVERWRITE ring buffer?
    T* MapDiscard(ID3D11DeviceContext* d3dDeviceContext);

    uint32_t GetNumElements() const { return m_Elements; }


private:
    uint32_t m_Elements;
};

template<class T>
inline StructuredBuffer<T>::StructuredBuffer(ID3D11Device* d3dDevice, uint32_t elements, uint32_t bindFlags, bool enableCounter, bool dynamic)
    : m_Elements(elements), 
    Buffer(d3dDevice, 
        CD3D11_BUFFER_DESC(sizeof(T) * elements, bindFlags,
        dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        dynamic ? D3D11_CPU_ACCESS_WRITE : 0,
        D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        sizeof(T)), 
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, elements),
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, elements, 0, D3D11_BUFFER_UAV_FLAG_COUNTER))
{
}

template <typename T>
T* StructuredBuffer<T>::MapDiscard(ID3D11DeviceContext* d3dDeviceContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    return static_cast<T*>(mappedResource.pData);
}

template <DXGI_FORMAT format>
struct TypedBuffer : public Buffer
{
public:
    TypedBuffer(ID3D11Device* d3dDevice, uint32_t numElems,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool dynamic = false);
    ~TypedBuffer() = default;

    // 不允许拷贝，允许移动
    TypedBuffer(const TypedBuffer&) = delete;
    TypedBuffer& operator=(const TypedBuffer&) = delete;
    TypedBuffer(TypedBuffer&&) = default;
    TypedBuffer& operator=(TypedBuffer&&) = default;

    uint32_t GetNumElements() const { return m_Elements; }

private:
    uint32_t m_Elements;
};

template<DXGI_FORMAT format>
TypedBuffer<format>::TypedBuffer(ID3D11Device* d3dDevice, uint32_t numElems, uint32_t bindFlags, bool dynamic)
    : m_Elements(numElems), Buffer(d3dDevice, CD3D11_BUFFER_DESC(
        GetFormatSize(format) * numElems, bindFlags,
        dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
        dynamic ? D3D11_CPU_ACCESS_WRITE : 0),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(m_pBuffer.Get(), format, 0, numElems),
        CD3D11_UNORDERED_ACCESS_VIEW_DESC(m_pBuffer.Get(), format, 0, numElems))
{
}

struct ByteAddressBuffer : public Buffer
{
public:
    ByteAddressBuffer(ID3D11Device* d3dDevice, uint32_t numUInt32s,
        uint32_t bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
        bool dynamic = false, bool indirectArgs = false);
    ~ByteAddressBuffer() = default;

    // 不允许拷贝，允许移动
    ByteAddressBuffer(const ByteAddressBuffer&) = delete;
    ByteAddressBuffer& operator=(const ByteAddressBuffer&) = delete;
    ByteAddressBuffer(ByteAddressBuffer&&) = default;
    ByteAddressBuffer& operator=(ByteAddressBuffer&&) = default;

    uint32_t GetNumUInt32s() const { return m_NumUInt32s; }

private:
    uint32_t m_NumUInt32s;
};

#endif


