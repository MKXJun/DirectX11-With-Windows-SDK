#include "SkyRender.h"
#include "Geometry.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;
using namespace Microsoft::WRL;

SkyRender::SkyRender(
	ID3D11Device * device,
	ID3D11DeviceContext * deviceContext,
	const std::wstring & cubemapFilename,
	float skySphereRadius,
	bool generateMips)
	: m_IndexCount()
{
	// 天空盒纹理加载
	if (cubemapFilename.substr(cubemapFilename.size() - 3) == L"dds")
	{
		HR(CreateDDSTextureFromFile(
			device,
			generateMips ? deviceContext : nullptr,
			cubemapFilename.c_str(),
			nullptr,
			m_pTextureCubeSRV.GetAddressOf()
		));
	}
	else
	{
		HR(CreateWICTexture2DCubeFromFile(
			device,
			deviceContext,
			cubemapFilename,
			nullptr,
			m_pTextureCubeSRV.GetAddressOf(),
			generateMips
		));
	}

	InitResource(device, skySphereRadius);
}

SkyRender::SkyRender(ID3D11Device * device,
	ID3D11DeviceContext * deviceContext,
	const std::vector<std::wstring>& cubemapFilenames,
	float skySphereRadius,
	bool generateMips)
	: m_IndexCount()
{
	// 天空盒纹理加载

	HR(CreateWICTexture2DCubeFromFile(
		device,
		deviceContext,
		cubemapFilenames,
		nullptr,
		m_pTextureCubeSRV.GetAddressOf(),
		generateMips
	));

	InitResource(device, skySphereRadius);
}

ID3D11ShaderResourceView * SkyRender::GetTextureCube()
{
	return m_pTextureCubeSRV.Get();
}

void SkyRender::Draw(ID3D11DeviceContext * deviceContext, SkyEffect & skyEffect, const Camera & camera)
{
	UINT strides[1] = { sizeof(XMFLOAT3) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	XMFLOAT3 pos = camera.GetPosition();
	skyEffect.SetWorldViewProjMatrix(XMMatrixTranslation(pos.x, pos.y, pos.z) * camera.GetViewProjXM());
	skyEffect.SetTextureCube(m_pTextureCubeSRV.Get());
	skyEffect.Apply(deviceContext);
	deviceContext->DrawIndexed(m_IndexCount, 0, 0);
}

void SkyRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	std::string texCubeName = name + ".CubeMapSRV";
	std::string vbName = name + ".VertexBuffer";
	std::string ibName = name + ".IndexBuffer";
	m_pTextureCubeSRV->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(texCubeName.length()), texCubeName.c_str());
	m_pVertexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(vbName.length()), vbName.c_str());
	m_pIndexBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(ibName.length()), ibName.c_str());
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

void SkyRender::InitResource(ID3D11Device * device, float skySphereRadius)
{
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

	HR(device->CreateBuffer(&vbd, &InitData, &m_pVertexBuffer));

	// 索引缓冲区创建
	m_IndexCount = (UINT)sphere.indexVec.size();

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(WORD) * m_IndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	InitData.pSysMem = sphere.indexVec.data();

	HR(device->CreateBuffer(&ibd, &InitData, &m_pIndexBuffer));

}
