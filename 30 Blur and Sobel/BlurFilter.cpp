#include "BlurFilter.h"
#include "d3dUtil.h"

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;



HRESULT BlurFilter::InitResource(ID3D11Device* device, UINT texWidth, UINT texHeight)
{
	// 防止重复初始化造成内存泄漏
	m_pTempSRV0.Reset();
	m_pTempSRV1.Reset();
	m_pTempUAV0.Reset();
	m_pTempUAV1.Reset();
	m_pBlurHorzCS.Reset();
	m_pBlurVertCS.Reset();

	m_Width = texWidth;
	m_Height = texHeight;
	HRESULT hr;
	
	// 创建纹理
	ComPtr<ID3D11Texture2D> texture0, texture1;
	D3D11_TEXTURE2D_DESC texDesc;

	texDesc.Width = texWidth;
	texDesc.Height = texHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	hr = device->CreateTexture2D(&texDesc, nullptr, texture0.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateTexture2D(&texDesc, nullptr, texture1.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建纹理对应的着色器资源视图
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;	// 使用所有的mip等级

	hr = device->CreateShaderResourceView(texture0.Get(), &srvDesc,
		m_pTempSRV0.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateShaderResourceView(texture1.Get(), &srvDesc,
		m_pTempSRV1.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建纹理对应的无序访问视图
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	
	hr = device->CreateUnorderedAccessView(texture0.Get(), &uavDesc,
		m_pTempUAV0.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateUnorderedAccessView(texture1.Get(), &uavDesc,
		m_pTempUAV1.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建常量缓冲区
	hr = CreateConstantBuffer(device, nullptr, sizeof m_CBSettings, m_pConstantBuffer.GetAddressOf());
	if (FAILED(hr))
		return hr;

	// 创建计算着色器
	ComPtr<ID3DBlob> blob;
	hr = CreateShaderFromFile(L"HLSL\\Blur_Horz_CS.cso", L"HLSL\\Blur_Horz_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pBlurHorzCS.GetAddressOf());
	if (FAILED(hr))
		return hr;

	hr = CreateShaderFromFile(L"HLSL\\Blur_Vert_CS.cso", L"HLSL\\Blur_Vert_CS.hlsl", "CS", "cs_5_0", blob.ReleaseAndGetAddressOf());
	if (FAILED(hr))
		return hr;
	hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pBlurVertCS.GetAddressOf());

	return hr;
}

HRESULT BlurFilter::SetWeights(float weights[2 * MaxBlurRadius + 1], int blurRadius)
{
	if (blurRadius < 0 || blurRadius > MaxBlurRadius)
		return E_INVALIDARG;

	ZeroMemory(&m_CBSettings, sizeof m_CBSettings);
	m_CBSettings.blurRadius = blurRadius;
	size_t numWeights = 2 * (size_t)blurRadius + 1;
	memcpy_s(m_CBSettings.weights, sizeof(float) * numWeights,
		weights, sizeof(float) * numWeights);

	return S_OK;
}

HRESULT BlurFilter::SetGaussianWeights(int blurRadius, float sigma)
{
	if (blurRadius < 0 || blurRadius > MaxBlurRadius || sigma == 0.0f)
		return E_INVALIDARG;

	float twoSigmaSq = 2.0f * sigma * sigma;

	float sum = 0.0f;

	ZeroMemory(&m_CBSettings, sizeof m_CBSettings);
	m_CBSettings.blurRadius = blurRadius;
	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		m_CBSettings.weights[blurRadius + i] = expf(-x * x / twoSigmaSq);

		sum += m_CBSettings.weights[blurRadius + i];
	}

	// 标准化权值使得权值和为1.0
	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		m_CBSettings.weights[blurRadius + i] /= sum;
	}

	return S_OK;
}

void BlurFilter::Execute(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* inputTex, ID3D11UnorderedAccessView* outputTex, UINT blurTimes)
{
	if (!deviceContext || !inputTex || !blurTimes)
		return;

	// 设置常量缓冲区
	D3D11_MAPPED_SUBRESOURCE mappedData;
	deviceContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
	memcpy_s(mappedData.pData, sizeof m_CBSettings, &m_CBSettings, sizeof m_CBSettings);
	deviceContext->Unmap(m_pConstantBuffer.Get(), 0);

	deviceContext->CSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	// 第一次模糊
	// 横向模糊
	deviceContext->CSSetShader(m_pBlurHorzCS.Get(), nullptr, 0);
	deviceContext->CSSetShaderResources(0, 1, &inputTex);
	deviceContext->CSSetUnorderedAccessViews(0, 1, m_pTempUAV0.GetAddressOf(), nullptr);

	deviceContext->Dispatch((UINT)ceilf(m_Width / 256.0f), m_Height, 1);
	deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	// 纵向模糊
	deviceContext->CSSetShader(m_pBlurVertCS.Get(), nullptr, 0);
	deviceContext->CSSetShaderResources(0, 1, m_pTempSRV0.GetAddressOf());
	if (blurTimes == 1 && outputTex)
		deviceContext->CSSetUnorderedAccessViews(0, 1, &outputTex, nullptr);
	else
		deviceContext->CSSetUnorderedAccessViews(0, 1, m_pTempUAV1.GetAddressOf(), nullptr);
	deviceContext->Dispatch(m_Width, (UINT)ceilf(m_Height / 256.0f), 1);
	deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

	// 剩余次数模糊
	while (--blurTimes)
	{
		// 横向模糊
		deviceContext->CSSetShader(m_pBlurHorzCS.Get(), nullptr, 0);
		deviceContext->CSSetShaderResources(0, 1, m_pTempSRV1.GetAddressOf());
		deviceContext->CSSetUnorderedAccessViews(0, 1, m_pTempUAV0.GetAddressOf(), nullptr);

		deviceContext->Dispatch((UINT)ceilf(m_Width / 256.0f), m_Height, 1);
		deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
		// 纵向模糊
		deviceContext->CSSetShader(m_pBlurVertCS.Get(), nullptr, 0);
		deviceContext->CSSetShaderResources(0, 1, m_pTempSRV0.GetAddressOf());
		if (blurTimes == 1 && outputTex)
			deviceContext->CSSetUnorderedAccessViews(0, 1, &outputTex, nullptr);
		else
			deviceContext->CSSetUnorderedAccessViews(0, 1, m_pTempUAV1.GetAddressOf(), nullptr);
		deviceContext->Dispatch(m_Width, (UINT)ceilf(m_Height / 256.0f), 1);

		
		deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	}
	// 解除剩余绑定
	deviceContext->CSSetShaderResources(0, 1, nullSRV);

}

ID3D11ShaderResourceView* BlurFilter::GetOutputTexture()
{
	return m_pTempSRV1.Get();
}

void BlurFilter::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	D3D11SetDebugObjectName(m_pConstantBuffer.Get(), name + ".ConstantBuffer");
	D3D11SetDebugObjectName(m_pTempSRV0.Get(), name + ".TempSRV0");
	D3D11SetDebugObjectName(m_pTempSRV1.Get(), name + ".TempSRV1");
	D3D11SetDebugObjectName(m_pTempUAV0.Get(), name + ".TempUAV0");
	D3D11SetDebugObjectName(m_pTempUAV1.Get(), name + ".TempUAV1");
	D3D11SetDebugObjectName(m_pBlurHorzCS.Get(), name + ".BlurHorz_CS");
	D3D11SetDebugObjectName(m_pBlurVertCS.Get(), name + ".BlurVert_CS");
#else
	UNREFERENCED_PARAMETER(name);
#endif
}

