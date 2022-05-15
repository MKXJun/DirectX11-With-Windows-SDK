#include "SobelFilter.h"
#include "d3dUtil.h"

#pragma warning(disable: 26812)

using namespace DirectX;
using namespace Microsoft::WRL;



HRESULT SobelFilter::InitResource(ID3D11Device* device, UINT texWidth, UINT texHeight)
{
    // 防止重复初始化造成内存泄漏
    m_pTempSRV.Reset();
    m_pTempUAV.Reset();
    m_pSobelCS.Reset();

    m_Width = texWidth;
    m_Height = texHeight;
    HRESULT hr;

    // 创建纹理
    ComPtr<ID3D11Texture2D> texture;
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

    hr = device->CreateTexture2D(&texDesc, nullptr, texture.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // 创建纹理对应的着色器资源视图
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = -1;	// 使用所有的mip等级

    hr = device->CreateShaderResourceView(texture.Get(), &srvDesc,
        m_pTempSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // 创建纹理对应的无序访问视图
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format = texDesc.Format;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    hr = device->CreateUnorderedAccessView(texture.Get(), &uavDesc,
        m_pTempUAV.GetAddressOf());
    if (FAILED(hr))
        return hr;

    // 创建计算着色器
    ComPtr<ID3DBlob> blob;
    hr = CreateShaderFromFile(L"HLSL\\Sobel_CS.cso", L"HLSL\\Sobel_CS.hlsl", "CS", "cs_5_0", blob.GetAddressOf());
    if (FAILED(hr))
        return hr;
    hr = device->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pSobelCS.GetAddressOf());
    return hr;
}

void SobelFilter::Execute(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* inputTex, ID3D11UnorderedAccessView* outputTex)
{
    if (!deviceContext || !inputTex)
        return;

    deviceContext->CSSetShader(m_pSobelCS.Get(), nullptr, 0);
    deviceContext->CSSetShaderResources(0, 1, &inputTex);
    if (outputTex)
        deviceContext->CSSetUnorderedAccessViews(0, 1, &outputTex, nullptr);
    else
        deviceContext->CSSetUnorderedAccessViews(0, 1, m_pTempUAV.GetAddressOf(), nullptr);
    deviceContext->Dispatch((UINT)ceilf(m_Width / 16.0f), (UINT)ceilf(m_Height / 16.0f), 1);

    // 解除绑定
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };
    deviceContext->CSSetShaderResources(0, 1, nullSRV);
    deviceContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
}

ID3D11ShaderResourceView* SobelFilter::GetOutputTexture()
{
    return m_pTempSRV.Get();
}

void SobelFilter::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    D3D11SetDebugObjectName(m_pTempSRV.Get(), name + ".TempSRV");
    D3D11SetDebugObjectName(m_pTempUAV.Get(), name + ".TempUAV");
    D3D11SetDebugObjectName(m_pSobelCS.Get(), name + ".Sobel_CS");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
