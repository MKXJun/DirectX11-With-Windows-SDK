#include "Texture2D.h"
#include "XUtil.h"
#include <cassert>

#pragma warning(disable: 26812)

using namespace Microsoft::WRL;

Texture2D::Texture2D(ID3D11Device* d3dDevice,
    UINT width, UINT height, DXGI_FORMAT format,
    UINT bindFlags,
    UINT mipLevels)
{
    InternalConstruct(d3dDevice, width, height, format, bindFlags, mipLevels, 1, 1, 0,
        D3D11_RTV_DIMENSION_TEXTURE2D, D3D11_UAV_DIMENSION_TEXTURE2D, D3D11_SRV_DIMENSION_TEXTURE2D);
}

Texture2D::Texture2D(ID3D11Device* d3dDevice,
    UINT width, UINT height, DXGI_FORMAT format,
    UINT bindFlags,
    const DXGI_SAMPLE_DESC& sampleDesc)
{
    // UAV's can't point to multisampled resources
    InternalConstruct(d3dDevice, width, height, format, bindFlags, 1, 1, sampleDesc.Count, sampleDesc.Quality,
        D3D11_RTV_DIMENSION_TEXTURE2DMS, D3D11_UAV_DIMENSION_UNKNOWN, D3D11_SRV_DIMENSION_TEXTURE2DMS);
}

Texture2D::Texture2D(ID3D11Device* d3dDevice,
    UINT width, UINT height, DXGI_FORMAT format,
    UINT bindFlags,
    UINT mipLevels, UINT arraySize)
{
    InternalConstruct(d3dDevice, width, height, format, bindFlags, mipLevels, arraySize, 1, 0,
        D3D11_RTV_DIMENSION_TEXTURE2DARRAY, D3D11_UAV_DIMENSION_TEXTURE2DARRAY, D3D11_SRV_DIMENSION_TEXTURE2DARRAY);
}

Texture2D::Texture2D(ID3D11Device* d3dDevice,
    UINT width, UINT height, DXGI_FORMAT format,
    UINT bindFlags,
    UINT arraySize, const DXGI_SAMPLE_DESC& sampleDesc)
{
    // UAV不能指向多重采样资源
    InternalConstruct(d3dDevice, width, height, format, bindFlags, 1, arraySize, sampleDesc.Count, sampleDesc.Quality,
        D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY, D3D11_UAV_DIMENSION_UNKNOWN, D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY);
}

void Texture2D::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
    std::string str = name + ".SRV";
    m_pTextureSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)str.length(), str.c_str());
    for (size_t i = 0; i < m_pRenderTargetElements.size(); ++i)
    {
        str = name + ".RTV[" + std::to_string(i) + "]";
        m_pRenderTargetElements[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)str.length(), str.c_str());
    }
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
    {
        str = name + ".SRV[" + std::to_string(i) + "]";
        m_pShaderResourceElements[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)str.length(), str.c_str());
    }
    for (size_t i = 0; i < m_pUnorderedAccessElements.size(); ++i)
    {
        str = name + ".UAV[" + std::to_string(i) + "]";
        m_pUnorderedAccessElements[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)str.length(), str.c_str());
    }
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

void Texture2D::InternalConstruct(ID3D11Device* d3dDevice,
    UINT width, UINT height, DXGI_FORMAT format,
    UINT bindFlags, UINT mipLevels, UINT arraySize,
    UINT sampleCount, UINT sampleQuality,
    D3D11_RTV_DIMENSION rtvDimension,
    D3D11_UAV_DIMENSION uavDimension,
    D3D11_SRV_DIMENSION srvDimension)
{
    m_pTexture.Reset();
    m_pTextureSRV.Reset();
    m_pRenderTargetElements.clear();
    m_pShaderResourceElements.clear();
    m_pUnorderedAccessElements.clear();

    CD3D11_TEXTURE2D_DESC desc(
        format,
        width, height, arraySize, mipLevels,
        bindFlags,
        D3D11_USAGE_DEFAULT, 0,
        sampleCount, sampleQuality,
        (mipLevels != 1 ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0));

    d3dDevice->CreateTexture2D(&desc, 0, m_pTexture.GetAddressOf());

    // 用实际产生的mip等级等数据更新
    m_pTexture->GetDesc(&desc);

    if (bindFlags & D3D11_BIND_RENDER_TARGET) {
        for (UINT i = 0; i < arraySize; ++i) {
            CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(
                rtvDimension,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11RenderTargetView> pRTV;
            d3dDevice->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, pRTV.GetAddressOf());
            m_pRenderTargetElements.push_back(pRTV);
        }
    }

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS) {
        // UAV不能指向多重采样资源！ 
        assert(uavDimension != D3D11_UAV_DIMENSION_UNKNOWN);

        for (UINT i = 0; i < arraySize; ++i) {
            CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(
                uavDimension,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11UnorderedAccessView> pUAV;
            d3dDevice->CreateUnorderedAccessView(m_pTexture.Get(), &uavDesc, pUAV.GetAddressOf());
            m_pUnorderedAccessElements.push_back(pUAV);
        }
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE) {
        // 整个资源
        CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(
            srvDimension,
            format,
            0, desc.MipLevels,  // Mips
            0, desc.ArraySize   // Array
        );

        d3dDevice->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pTextureSRV.GetAddressOf());

        // 单个子资源
        for (UINT i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                srvDimension,
                format,
                0, 1,  // Mips
                i, 1   // Array
            );

            ComPtr<ID3D11ShaderResourceView> pSRV;
            d3dDevice->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}

Depth2D::Depth2D(ID3D11Device* d3dDevice,
    UINT width, UINT height,
    UINT bindFlags,
    bool stencil)
{
    InternalConstruct(d3dDevice, width, height, bindFlags, 1, 1, 0,
        D3D11_DSV_DIMENSION_TEXTURE2D, D3D11_SRV_DIMENSION_TEXTURE2D, stencil);
}

Depth2D::Depth2D(ID3D11Device* d3dDevice,
    UINT width, UINT height,
    UINT bindFlags,
    const DXGI_SAMPLE_DESC& sampleDesc,
    bool stencil)
{
    InternalConstruct(d3dDevice, width, height, bindFlags, 1, sampleDesc.Count, sampleDesc.Quality,
        D3D11_DSV_DIMENSION_TEXTURE2DMS, D3D11_SRV_DIMENSION_TEXTURE2DMS, stencil);
}

Depth2D::Depth2D(ID3D11Device* d3dDevice,
    UINT width, UINT height,
    UINT bindFlags,
    UINT arraySize,
    bool stencil)
{
    InternalConstruct(d3dDevice, width, height, bindFlags, arraySize, 1, 0,
        D3D11_DSV_DIMENSION_TEXTURE2DARRAY, D3D11_SRV_DIMENSION_TEXTURE2DARRAY, stencil);
}

Depth2D::Depth2D(ID3D11Device* d3dDevice,
    UINT width, UINT height,
    UINT bindFlags,
    UINT arraySize,
    const DXGI_SAMPLE_DESC& sampleDesc,
    bool stencil)
{
    InternalConstruct(d3dDevice, width, height, bindFlags, arraySize, sampleDesc.Count, sampleDesc.Quality,
        D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY, D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, stencil);
}

void Depth2D::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
    std::string str = name + ".SRV";
    m_pTextureSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
    for (size_t i = 0; i < m_pDepthStencilElements.size(); ++i)
    {
        str = name + ".DSV[" + std::to_string(i) + "]";
        m_pDepthStencilElements[i]->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
    }
#else
    UNREFERENCED_PARAMETER(name);
#endif // GRAPHICS_DEBUGGER_OBJECT_NAME
}

void Depth2D::InternalConstruct(ID3D11Device* d3dDevice,
    UINT width, UINT height,
    UINT bindFlags, UINT arraySize,
    UINT sampleCount, UINT sampleQuality,
    D3D11_DSV_DIMENSION dsvDimension,
    D3D11_SRV_DIMENSION srvDimension,
    bool stencil)
{
    m_pTexture.Reset();
    m_pTextureSRV.Reset();
    m_pDepthStencilElements.clear();
    m_pShaderResourceElements.clear();

    CD3D11_TEXTURE2D_DESC desc(
        stencil ? DXGI_FORMAT_R32G8X24_TYPELESS : DXGI_FORMAT_R32_TYPELESS,
        width, height, arraySize, 1,
        bindFlags,
        D3D11_USAGE_DEFAULT, 0,
        sampleCount, sampleQuality);

    d3dDevice->CreateTexture2D(&desc, 0, m_pTexture.GetAddressOf());

    if (bindFlags & D3D11_BIND_DEPTH_STENCIL) {
        for (UINT i = 0; i < arraySize; ++i) {
            CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDesc(
                dsvDimension,
                stencil ? DXGI_FORMAT_D32_FLOAT_S8X24_UINT : DXGI_FORMAT_D32_FLOAT,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11DepthStencilView> pDSV;
            d3dDevice->CreateDepthStencilView(m_pTexture.Get(), &depthStencilDesc, pDSV.GetAddressOf());
            m_pDepthStencilElements.push_back(pDSV);
        }
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE) {
        CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceDesc(
            srvDimension,
            stencil ? DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS : DXGI_FORMAT_R32_FLOAT,
            0, 1,           // Mips
            0, arraySize    // Array
        );

        d3dDevice->CreateShaderResourceView(m_pTexture.Get(), &shaderResourceDesc, m_pTextureSRV.GetAddressOf());

        // 单个子资源
        for (UINT i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                srvDimension,
                stencil ? DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS : DXGI_FORMAT_R32_FLOAT,
                0, 1,  // Mips
                i, 1   // Array
            );

            ComPtr<ID3D11ShaderResourceView> pSRV;
            d3dDevice->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}
