#include "ShadowRender.h"
#include "d3dUtil.h"

HRESULT ShadowRender::InitResource(ID3D11Device* device, UINT width, UINT height)
{
    m_pDepthBuffer.Reset();
    m_pDepthBufferDSV.Reset();
    m_pDepthBufferSRV.Reset();

    m_Width = width;
    m_Height = height;

    m_ViewPort = CD3D11_VIEWPORT(0.0f, 0.0f, (FLOAT)width, (FLOAT)height);



    CD3D11_TEXTURE2D_DESC texDesc(m_Format, m_Width, m_Height, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL);
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, m_pDepthBuffer.GetAddressOf());
    if (FAILED(hr))
        return hr;

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(m_pDepthBuffer.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    hr = device->CreateShaderResourceView(m_pDepthBuffer.Get(), &srvDesc, m_pDepthBufferSRV.GetAddressOf());
    if (FAILED(hr))
        return hr;
    
    CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(m_pDepthBuffer.Get(), D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
    hr = device->CreateDepthStencilView(m_pDepthBuffer.Get(), &dsvDesc, m_pDepthBufferDSV.GetAddressOf());
    return hr;
}

ID3D11ShaderResourceView* ShadowRender::GetSRV()
{
    return m_pDepthBufferSRV.Get();
}

void ShadowRender::Begin(ID3D11DeviceContext* deviceContext)
{
    UINT numViewPorts = 1;
    deviceContext->RSGetViewports(&numViewPorts, &m_CachedViewPort);
    deviceContext->OMGetRenderTargets(1, m_pCachedRTV.GetAddressOf(), m_pCachedDSV.GetAddressOf());

    deviceContext->ClearDepthStencilView(m_pDepthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    deviceContext->RSSetViewports(1, &m_ViewPort);
    deviceContext->OMSetRenderTargets(0, nullptr, m_pDepthBufferDSV.Get());


}

void ShadowRender::End(ID3D11DeviceContext* deviceContext)
{
    deviceContext->RSSetViewports(1, &m_CachedViewPort);
    deviceContext->OMSetRenderTargets(1, m_pCachedRTV.GetAddressOf(), m_pCachedDSV.Get());

    m_pCachedDSV.Reset();
    m_pCachedRTV.Reset();
}

void ShadowRender::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    D3D11SetDebugObjectName(m_pDepthBuffer.Get(), name + ".DepthBuffer");
    D3D11SetDebugObjectName(m_pDepthBufferSRV.Get(), name + ".DepthBufferSRV");
    D3D11SetDebugObjectName(m_pDepthBufferDSV.Get(), name + ".DepthBufferDSV");
#endif
}
