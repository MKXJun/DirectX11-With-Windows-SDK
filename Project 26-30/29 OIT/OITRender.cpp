#include "OITRender.h"
#include "RenderStates.h"
#include "Geometry.h"
#include "d3dUtil.h"

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;

HRESULT OITRender::InitResource(ID3D11Device* device, UINT width, UINT height, UINT multiple)
{
	// 防止重复初始化造成内存泄漏
	m_pFLBuffer.Reset();
	m_pStartOffsetBuffer.Reset();
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	m_pConstantBuffer.Reset();
	m_pFLBufferSRV.Reset();
	m_pStartOffsetBufferSRV.Reset();
	m_pFLBufferUAV.Reset();
	m_pStartOffsetBufferUAV.Reset();
	m_pOITRenderPS.Reset();
	m_pOITRenderVS.Reset();
	m_pOITStorePS.Reset();


	m_FrameWidth = width;
	m_FrameHeight = height;

	HRESULT hr;

	auto frameMesh = Geometry::Create2DShow<VertexPos>();
	m_IndexCount = (UINT)frameMesh.indexVec.size();
	// 创建顶点缓冲区
	hr = CreateVertexBuffer(device, frameMesh.vertexVec.data(), (UINT)frameMesh.vertexVec.size() * sizeof(VertexPos),
		m_pVertexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 创建索引缓冲区
	hr = CreateIndexBuffer(device, frameMesh.indexVec.data(), (UINT)frameMesh.indexVec.size() * sizeof(DWORD),
		m_pIndexBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建常量缓冲区
	UINT initVals[4] = { m_FrameWidth, m_FrameHeight, 0, 0 };
	hr = CreateConstantBuffer(device, initVals, 16, m_pConstantBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 创建结构化缓冲区
	hr = CreateStructuredBuffer(device, nullptr, 12 * width * height * multiple, 12,
		m_pFLBuffer.GetAddressOf(), false, true);
	if (FAILED(hr))
		return hr;
	// 创建字节地址缓冲区
	hr = CreateRawBuffer(device, nullptr, 4 * width * height,
		m_pStartOffsetBuffer.GetAddressOf(), false, true);
	if (FAILED(hr))
		return hr;

	// 创建着色器资源视图
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = width * height * multiple;
	hr = device->CreateShaderResourceView(m_pFLBuffer.Get(), &srvDesc,
		m_pFLBufferSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.NumElements = width * height;
	srvDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
	hr = device->CreateShaderResourceView(m_pStartOffsetBuffer.Get(), &srvDesc,
		m_pStartOffsetBufferSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建无序访问视图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = width * height * multiple;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER;
	hr = device->CreateUnorderedAccessView(m_pFLBuffer.Get(), &uavDesc, m_pFLBufferUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	uavDesc.Buffer.NumElements = width * height;
	uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;
	hr = device->CreateUnorderedAccessView(m_pStartOffsetBuffer.Get(), &uavDesc, m_pStartOffsetBufferUAV.GetAddressOf());
	if (FAILED(hr))
		return hr;


	ComPtr<ID3DBlob> blob;
	// 创建顶点着色器
	hr = CreateShaderFromFile(L"HLSL\\OIT_Render_VS.cso", L"HLSL\\OIT_Render_VS.hlsl", "VS", "vs_5_0", blob.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pOITRenderVS.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 创建顶点输入布局
	hr = device->CreateInputLayout(VertexPos::inputLayout, 1, blob->GetBufferPointer(), blob->GetBufferSize(),
		m_pInputLayout.GetAddressOf());
	if (FAILED(hr))
		return hr;


	// 创建像素着色器
	hr = CreateShaderFromFile(L"HLSL\\OIT_Render_PS.cso", L"HLSL\\OIT_Render_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pOITRenderPS.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = CreateShaderFromFile(L"HLSL\\OIT_Store_PS.cso", L"HLSL\\OIT_Store_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pOITStorePS.GetAddressOf());

	return hr;
}

void OITRender::BeginDefaultStore(ID3D11DeviceContext* deviceContext)
{
	deviceContext->RSSetState(RenderStates::RSNoCull.Get());

	UINT numClassInstances = 0;
	deviceContext->PSGetShader(m_pCachePS.GetAddressOf(), nullptr, &numClassInstances);
	deviceContext->PSSetShader(m_pOITStorePS.Get(), nullptr, 0);

	// 初始化UAV
	UINT magicValue[1] = { 0xFFFFFFFF };
	deviceContext->ClearUnorderedAccessViewUint(m_pFLBufferUAV.Get(), magicValue);
	deviceContext->ClearUnorderedAccessViewUint(m_pStartOffsetBufferUAV.Get(), magicValue);
	// UAV绑定到像素着色阶段
	ID3D11UnorderedAccessView* pUAVs[2] = { m_pFLBufferUAV.Get(), m_pStartOffsetBufferUAV.Get() };
	UINT initCounts[2] = { 0, 0 };
	deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
		nullptr, nullptr, 1, 2, pUAVs, initCounts);

	// 关闭深度写入
	deviceContext->OMSetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
	// 设置常量缓冲区
	deviceContext->PSSetConstantBuffers(6, 1, m_pConstantBuffer.GetAddressOf());
}

void OITRender::EndStore(ID3D11DeviceContext* deviceContext)
{
	// 恢复渲染状态
	deviceContext->PSSetShader(m_pCachePS.Get(), nullptr, 0);
	ComPtr<ID3D11RenderTargetView> currRTV;
	ComPtr<ID3D11DepthStencilView> currDSV;
	ID3D11UnorderedAccessView* pUAVs[2] = { nullptr, nullptr };
	deviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL,
		nullptr, nullptr, 1, 2, pUAVs, nullptr);
	m_pCachePS.Reset();
}

void OITRender::Draw(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* background)
{

	UINT strides[1] = { sizeof(VertexPos) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	deviceContext->IASetInputLayout(m_pInputLayout.Get());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	deviceContext->VSSetShader(m_pOITRenderVS.Get(), nullptr, 0);
	deviceContext->PSSetShader(m_pOITRenderPS.Get(), nullptr, 0);

	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->RSSetState(nullptr);

	ID3D11ShaderResourceView* pSRVs[3] = {
		m_pFLBufferSRV.Get(), m_pStartOffsetBufferSRV.Get(), background };
	deviceContext->PSSetShaderResources(0, 3, pSRVs);
	deviceContext->PSSetConstantBuffers(6, 1, m_pConstantBuffer.GetAddressOf());

	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

	deviceContext->DrawIndexed(m_IndexCount, 0, 0);

	// 绘制完成后卸下绑定的资源即可
	pSRVs[0] = pSRVs[1] = pSRVs[2] = nullptr;
	deviceContext->PSSetShaderResources(0, 3, pSRVs);

}

void OITRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), name + ".VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), name + ".IndexBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffer.Get(), name + ".ConstantBuffer");
	D3D11SetDebugObjectName(m_pFLBuffer.Get(), name + ".FragmentLinkBuffer");
	D3D11SetDebugObjectName(m_pStartOffsetBuffer.Get(), name + ".StartOffsetBuffer");

	D3D11SetDebugObjectName(m_pFLBufferSRV.Get(), name + ".FragmentLinkBufferSRV");
	D3D11SetDebugObjectName(m_pFLBufferUAV.Get(), name + ".FragmentLinkBufferUAV");
	D3D11SetDebugObjectName(m_pStartOffsetBufferSRV.Get(), name + ".StartOffsetBufferSRV");
	D3D11SetDebugObjectName(m_pStartOffsetBufferUAV.Get(), name + ".StartOffsetBufferUAV");

	D3D11SetDebugObjectName(m_pInputLayout.Get(), name + ".InputLayout");
	D3D11SetDebugObjectName(m_pOITRenderVS.Get(), name + ".OIT_Render_VS");
	D3D11SetDebugObjectName(m_pOITRenderPS.Get(), name + ".OIT_Store_PS");
	D3D11SetDebugObjectName(m_pOITStorePS.Get(), name + ".OIT_Store_PS");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}
