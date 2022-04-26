
#pragma once

#ifndef BUFFER_H
#define BUFFER_H

#include "WinMin.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <string>

// 注意：确保T与着色器中结构体的大小/布局相同
template<class T>
class StructuredBuffer
{
public:
	StructuredBuffer(ID3D11Device* d3dDevice, int elements,
		UINT bindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,
		bool dynamic = false);
	~StructuredBuffer() = default;

	// 不允许拷贝，允许移动
	StructuredBuffer(const StructuredBuffer&) = delete;
	StructuredBuffer& operator=(const StructuredBuffer&) = delete;
	StructuredBuffer(StructuredBuffer&&) = default;
	StructuredBuffer& operator=(StructuredBuffer&&) = default;

	ID3D11Buffer* GetBuffer() { return m_pBuffer.Get(); }
	ID3D11UnorderedAccessView* GetUnorderedAccess() { return m_pUnorderedAccess.Get(); }
	ID3D11ShaderResourceView* GetShaderResource() { return m_pShaderResource.Get(); }

	// 仅支持动态缓冲区
	// TODO: Support NOOVERWRITE ring buffer?
	T* MapDiscard(ID3D11DeviceContext* d3dDeviceContext);
	void Unmap(ID3D11DeviceContext* d3dDeviceContext);

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	template<class Type>
	using ComPtr = Microsoft::WRL::ComPtr<Type>;

	int m_Elements;
	ComPtr<ID3D11Buffer> m_pBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pShaderResource;
	ComPtr<ID3D11UnorderedAccessView> m_pUnorderedAccess;
};

template<class T>
inline StructuredBuffer<T>::StructuredBuffer(ID3D11Device* d3dDevice, int elements, UINT bindFlags, bool dynamic)
	: m_Elements(elements)
{
	CD3D11_BUFFER_DESC desc(sizeof(T) * elements, bindFlags,
		dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT,
		dynamic ? D3D11_CPU_ACCESS_WRITE : 0,
		D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
		sizeof(T));

	d3dDevice->CreateBuffer(&desc, nullptr, m_pBuffer.GetAddressOf());

	if (bindFlags & D3D11_BIND_UNORDERED_ACCESS) {
		d3dDevice->CreateUnorderedAccessView(m_pBuffer.Get(), 0, m_pUnorderedAccess.GetAddressOf());
	}

	if (bindFlags & D3D11_BIND_SHADER_RESOURCE) {
		d3dDevice->CreateShaderResourceView(m_pBuffer.Get(), 0, m_pShaderResource.GetAddressOf());
	}
}

template <typename T>
T* StructuredBuffer<T>::MapDiscard(ID3D11DeviceContext* d3dDeviceContext)
{
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	d3dDeviceContext->Map(m_pBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	return static_cast<T*>(mappedResource.pData);
}


template <typename T>
void StructuredBuffer<T>::Unmap(ID3D11DeviceContext* d3dDeviceContext)
{
	d3dDeviceContext->Unmap(m_pBuffer.Get(), 0);
}

template<class T>
inline void StructuredBuffer<T>::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG))

	m_pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.length()), name.c_str());
	if (m_pShaderResource)
	{
		std::string srvName = name + ".SRV";
		m_pShaderResource->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(srvName.length()), srvName.c_str());
	}
	if (m_pUnorderedAccess)
	{
		std::string uavName = name + ".UAV";
		m_pUnorderedAccess->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(uavName.length()), uavName.c_str());
	}
		
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

#endif