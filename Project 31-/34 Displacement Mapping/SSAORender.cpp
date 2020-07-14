#include "SSAORender.h"
#include "d3dUtil.h"
#include "Vertex.h"
#include <random>

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;


HRESULT SSAORender::InitResource(ID3D11Device* device, int width, int height, float fovY, float farZ)
{
	if (!device)
	{
		return E_INVALIDARG;
	}

	// 防止重复初始化造成内存泄漏
	m_pScreenQuadVB.Reset();
	m_pScreenQuadIB.Reset();
	m_pRandomVectorSRV.Reset();

	HRESULT hr;
	
	hr = OnResize(device, width, height, fovY, farZ);
	if (FAILED(hr))
		return hr;

	hr = BuildFullScreenQuad(device);
	if (FAILED(hr))
		return hr;

	BuildOffsetVectors();

	hr = BuildRandomVectorTexture(device);
	return hr;
}

HRESULT SSAORender::OnResize(ID3D11Device* device, int width, int height, float fovY, float farZ)
{
	if (!device)
	{
		return E_INVALIDARG;
	}

	// 防止重复初始化造成内存泄漏
	m_pRandomVectorSRV.Reset();
	m_pAmbientRTV0.Reset();
	m_pAmbientRTV1.Reset();
	m_pAmbientSRV0.Reset();
	m_pAmbientSRV1.Reset();

	m_Width = width;
	m_Height = height;

	// 我们以一半的分辨率来渲染到SSAO图
	m_AmbientMapViewPort = CD3D11_VIEWPORT(0.0f, 0.0f, width / 2.0f, height / 2.0f, 0.0f, 1.0f);

	BuildFrustumFarCorners(fovY, farZ);
	HRESULT hr = BuildTextureViews(device, width, height);
	return hr;
}

void SSAORender::Begin(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* dsv, bool enableAlphaClip)
{
	// 缓存RTV、DSV和视口
	deviceContext->OMGetRenderTargets(1, m_pCachedRTV.GetAddressOf(), m_pCachedDSV.GetAddressOf());
	UINT numViewPorts = 1;
	deviceContext->RSGetViewports(&numViewPorts, &m_CachedViewPort);

	// 将指定的DSV绑定到管线上
	ID3D11RenderTargetView* RTVs[1] = { m_pNormalDepthRTV.Get() };
	deviceContext->OMSetRenderTargets(1, RTVs, dsv);

	// 使用观察空间法向量(0, 0, -1)和非常大的深度值来清空RTV
	static const float clearColor[4] = { 0.0f, 0.0f, -1.0f, 1e5f };
	deviceContext->ClearRenderTargetView(m_pNormalDepthRTV.Get(), clearColor);
}

void SSAORender::End(ID3D11DeviceContext* deviceContext)
{
	// 恢复RTV、DSV和视口
	deviceContext->OMSetRenderTargets(1, m_pCachedRTV.GetAddressOf(), m_pCachedDSV.Get());
	deviceContext->RSSetViewports(1, &m_CachedViewPort);

	m_pCachedRTV.Reset();
	m_pCachedDSV.Reset();
}

void SSAORender::ComputeSSAO(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, const Camera& camera)
{
	// 将SSAO图作为RTV绑定
	// 这里我们不绑定深度/模板缓冲区，因为我们不需要使用，并且深度测试将不会进行，这也是我们想要的
	ID3D11RenderTargetView* RTVs[1] = { m_pAmbientRTV0.Get() };
	deviceContext->OMSetRenderTargets(1, RTVs, nullptr);
	static const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceContext->ClearRenderTargetView(m_pAmbientRTV0.Get(), black);
	deviceContext->RSSetViewports(1, &m_AmbientMapViewPort);

	ssaoEffect.SetRenderSSAOMap(deviceContext, 14);

	ssaoEffect.SetProjMatrix(camera.GetProjXM());
	ssaoEffect.SetOffsetVectors(m_Offsets);
	ssaoEffect.SetFrustumCorners(m_FrustumFarCorner);
	ssaoEffect.SetTextureNormalDepth(m_pNormalDepthSRV.Get());
	ssaoEffect.SetTextureRandomVec(m_pRandomVectorSRV.Get());

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetVertexBuffers(0, 1, m_pScreenQuadVB.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pScreenQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);

	ssaoEffect.Apply(deviceContext);
	deviceContext->DrawIndexed(6, 0, 0);
}

void SSAORender::BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, int blurCount)
{
	// 每次模糊需要进行一次水平模糊和垂直模糊
	for (int i = 0; i < blurCount; ++i)
	{
		BlurAmbientMap(deviceContext, ssaoEffect, m_pAmbientSRV0.Get(), m_pAmbientRTV1.Get(), true);
		BlurAmbientMap(deviceContext, ssaoEffect, m_pAmbientSRV1.Get(), m_pAmbientRTV0.Get(), false);
	}
}

ID3D11ShaderResourceView* SSAORender::GetAmbientTexture()
{
	return m_pAmbientSRV0.Get();
}

void SSAORender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	D3D11SetDebugObjectName(m_pRandomVectorSRV.Get(), name + ".RandomVectorSRV");
	D3D11SetDebugObjectName(m_pNormalDepthRTV.Get(), name + ".NormalDepthRTV");
	D3D11SetDebugObjectName(m_pNormalDepthSRV.Get(), name + ".NormalDepthSRV");
	D3D11SetDebugObjectName(m_pScreenQuadVB.Get(), name + ".ScreenQuadVB");
	D3D11SetDebugObjectName(m_pScreenQuadIB.Get(), name + ".ScreenQuadIB");
	D3D11SetDebugObjectName(m_pAmbientRTV0.Get(), name + ".AmbientRTV0");
	D3D11SetDebugObjectName(m_pAmbientSRV0.Get(), name + ".AmbientSRV0");
	D3D11SetDebugObjectName(m_pAmbientRTV1.Get(), name + ".AmbientRTV1");
	D3D11SetDebugObjectName(m_pAmbientSRV1.Get(), name + ".AmbientSRV1");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

void SSAORender::BuildFrustumFarCorners(float fovY, float farZ)
{
	float aspect = (float)m_Width / (float)m_Height;

	float halfHeight = farZ * tanf(0.5f * fovY);
	float halfWidth = aspect * halfHeight;

	m_FrustumFarCorner[0] = XMFLOAT4(-halfWidth, -halfHeight, farZ, 0.0f);
	m_FrustumFarCorner[1] = XMFLOAT4(-halfWidth, +halfHeight, farZ, 0.0f);
	m_FrustumFarCorner[2] = XMFLOAT4(+halfWidth, +halfHeight, farZ, 0.0f);
	m_FrustumFarCorner[3] = XMFLOAT4(+halfWidth, -halfHeight, farZ, 0.0f);
}

void SSAORender::BuildOffsetVectors()
{
	// 从14个均匀分布的向量开始。我们选择立方体的8个角点，并沿着立方体的每个面选取中心点
	// 我们总是让这些点以相对另一边的形式交替出现。这种办法可以在我们选择少于14个采样点
	// 时仍然能够让向量均匀散开

	// 8个立方体角点向量
	m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6个面中心点向量
	m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);


	// 初始化随机数数据
	std::mt19937 randEngine;
	randEngine.seed(std::random_device()());
	std::uniform_real_distribution<float> randF(0.25f, 1.0f);
	for (int i = 0; i < 14; ++i)
	{
		// 创建长度范围在[0.25, 1.0]内的随机长度的向量
		float s = randF(randEngine);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

		XMStoreFloat4(&m_Offsets[i], v);
	}
}

HRESULT SSAORender::BuildTextureViews(ID3D11Device* device, int width, int height)
{

	CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height,
		1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);

	//
	// 创建法向量/深度纹理及对应的SRV与RTV
	//
	ComPtr<ID3D11Texture2D> normalDepthTex;
	HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, normalDepthTex.GetAddressOf());
	if (FAILED(hr))
		return hr;
	// 传nullptr创建默认视图以访问完整资源
	hr = device->CreateShaderResourceView(normalDepthTex.Get(), nullptr, m_pNormalDepthSRV.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateRenderTargetView(normalDepthTex.Get(), nullptr, m_pNormalDepthRTV.GetAddressOf());
	if (FAILED(hr))
		return hr;

	//
	// 以一半的分辨率来创建SSAO图
	//
	texDesc.Width = width / 2;
	texDesc.Height = height / 2;
	texDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ComPtr<ID3D11Texture2D> ambientTex0, ambientTex1;
	hr = device->CreateTexture2D(&texDesc, nullptr, ambientTex0.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateShaderResourceView(ambientTex0.Get(), nullptr, m_pAmbientSRV0.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateRenderTargetView(ambientTex0.Get(), nullptr, m_pAmbientRTV0.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = device->CreateTexture2D(&texDesc, nullptr, ambientTex1.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateShaderResourceView(ambientTex1.Get(), nullptr, m_pAmbientSRV1.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateRenderTargetView(ambientTex1.Get(), nullptr, m_pAmbientRTV1.GetAddressOf());

	return hr;
}

HRESULT SSAORender::BuildFullScreenQuad(ID3D11Device* device)
{
	// 创建顶点缓冲区
	VertexPosNormalTex v[4];

	v[0].pos = XMFLOAT3(-1.0f, -1.0f, 0.0f);
	v[1].pos = XMFLOAT3(-1.0f, +1.0f, 0.0f);
	v[2].pos = XMFLOAT3(+1.0f, +1.0f, 0.0f);
	v[3].pos = XMFLOAT3(+1.0f, -1.0f, 0.0f);

	// 使用Normal.x来存储视锥体远平面角点的索引
	v[0].normal = XMFLOAT3(0.0f, 0.0f, 0.0f);
	v[1].normal = XMFLOAT3(1.0f, 0.0f, 0.0f);
	v[2].normal = XMFLOAT3(2.0f, 0.0f, 0.0f);
	v[3].normal = XMFLOAT3(3.0f, 0.0f, 0.0f);

	v[0].tex = XMFLOAT2(0.0f, 1.0f);
	v[1].tex = XMFLOAT2(0.0f, 0.0f);
	v[2].tex = XMFLOAT2(1.0f, 0.0f);
	v[3].tex = XMFLOAT2(1.0f, 1.0f);

	HRESULT hr = CreateVertexBuffer(device, v, sizeof v, m_pScreenQuadVB.GetAddressOf());

	// 创建索引缓冲区
	DWORD indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	hr = CreateIndexBuffer(device, indices, sizeof indices, m_pScreenQuadIB.GetAddressOf());
	return hr;
}

HRESULT SSAORender::BuildRandomVectorTexture(ID3D11Device* device)
{
	CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_R8G8B8A8_UNORM, 256, 256, 1, 1, 
		D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_IMMUTABLE);
	
	D3D11_SUBRESOURCE_DATA initData = {};
	std::vector<XMCOLOR> randomVectors(256 * 256);

	// 初始化随机数数据
	std::mt19937 randEngine;
	randEngine.seed(std::random_device()());
	std::uniform_real_distribution<float> randF(0.0f, 1.0f);
	for (int i = 0; i < 256 * 256; ++i)
	{
		randomVectors[i] = XMCOLOR(randF(randEngine), randF(randEngine), randF(randEngine), 0.0f);
	}
	initData.pSysMem = randomVectors.data();
	initData.SysMemPitch = 256 * sizeof(XMCOLOR);

	HRESULT hr;
	ComPtr<ID3D11Texture2D> tex;
	hr = device->CreateTexture2D(&texDesc, &initData, tex.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = device->CreateShaderResourceView(tex.Get(), nullptr, m_pRandomVectorSRV.GetAddressOf());
	return hr;
}

void SSAORender::BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, 
	ID3D11ShaderResourceView* inputSRV, ID3D11RenderTargetView* outputRTV, bool horzBlur)
{
	ID3D11RenderTargetView* RTVs[1] = { outputRTV };
	deviceContext->OMSetRenderTargets(1, RTVs, nullptr);
	deviceContext->ClearRenderTargetView(outputRTV, reinterpret_cast<const float*>(&Colors::Black));
	deviceContext->RSSetViewports(1, &m_AmbientMapViewPort);

	ssaoEffect.SetRenderBilateralBlur(deviceContext, horzBlur);

	ssaoEffect.SetTextureNormalDepth(m_pNormalDepthSRV.Get());
	ssaoEffect.SetTextureBlur(inputSRV);

	UINT strides[1] = { sizeof(VertexPosNormalTex) };
	UINT offsets[1] = { 0 };
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetVertexBuffers(0, 1, m_pScreenQuadVB.GetAddressOf(), strides, offsets);
	deviceContext->IASetIndexBuffer(m_pScreenQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);
	
	ssaoEffect.Apply(deviceContext);
	deviceContext->DrawIndexed(6, 0, 0);

	// 解除绑定在管线上的SRV
	ssaoEffect.SetTextureBlur(nullptr);
	ssaoEffect.Apply(deviceContext);
}




