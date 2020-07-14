#include "SkyRender.h"
#include "Geometry.h"
#include "d3dUtil.h"

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;

HRESULT SkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::wstring& cubemapFilename, float skySphereRadius, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pIndexBuffer.Reset();
	m_pVertexBuffer.Reset();
	m_pTextureCubeSRV.Reset();

	HRESULT hr;
	// 天空盒纹理加载
	if (cubemapFilename.substr(cubemapFilename.size() - 3) == L"dds")
	{
		hr = CreateDDSTextureFromFile(device,
			generateMips ? deviceContext : nullptr,
			cubemapFilename.c_str(),
			nullptr,
			m_pTextureCubeSRV.GetAddressOf());
	}
	else
	{
		hr = CreateWICTexture2DCubeFromFile(device,
			deviceContext,
			cubemapFilename,
			nullptr,
			m_pTextureCubeSRV.GetAddressOf(),
			generateMips);
	}

	if (FAILED(hr))
		return hr;

	return InitResource(device, skySphereRadius);
}

HRESULT SkyRender::InitResource(ID3D11Device* device, ID3D11DeviceContext* deviceContext, const std::vector<std::wstring>& cubemapFilenames, float skySphereRadius, bool generateMips)
{
	// 防止重复初始化造成内存泄漏
	m_pIndexBuffer.Reset();
	m_pVertexBuffer.Reset();
	m_pTextureCubeSRV.Reset();

	HRESULT hr;
	// 天空盒纹理加载
	hr = CreateWICTexture2DCubeFromFile(device,
		deviceContext,
		cubemapFilenames,
		nullptr,
		m_pTextureCubeSRV.GetAddressOf(),
		generateMips);
	if (FAILED(hr))
		return hr;

	return InitResource(device, skySphereRadius);
}

ID3D11ShaderResourceView* SkyRender::GetTextureCube()
{
	return m_pTextureCubeSRV.Get();
}

void SkyRender::Draw(ID3D11DeviceContext* deviceContext, SkyEffect& skyEffect, const Camera& camera)
{
	UINT strides[1] = { sizeof(XMFLOAT3) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// 抹除平移分量，避免摄像机移动带来天空盒抖动
	XMMATRIX V = camera.GetViewXM();
	V.r[3] = g_XMIdentityR3;
	skyEffect.SetWorldViewProjMatrix(V * camera.GetProjXM());
	skyEffect.SetTextureCube(m_pTextureCubeSRV.Get());
	skyEffect.Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void SkyRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	// 先清空可能存在的名称
	D3D11SetDebugObjectName(m_pTextureCubeSRV.Get(), nullptr);

	D3D11SetDebugObjectName(m_pTextureCubeSRV.Get(), name + ".CubeMapSRV");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

HRESULT SkyRender::InitResource(ID3D11Device* device, float skySphereRadius)
{
	HRESULT hr;
	auto sphere = Geometry::CreateSphere<VertexPos>(skySphereRadius);

	// 顶点缓冲区创建
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(XMFLOAT3) * (UINT)sphere.vertexVec.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = sphere.vertexVec.data();

	hr = device->CreateBuffer(&vbd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// 索引缓冲区创建
	m_IndexCount = (UINT)sphere.indexVec.size();

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(DWORD) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	InitData.pSysMem = sphere.indexVec.data();

	return device->CreateBuffer(&ibd, &InitData, &m_pIndexBuffer);
}