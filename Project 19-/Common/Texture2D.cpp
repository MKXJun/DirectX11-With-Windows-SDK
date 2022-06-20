#include "Texture2D.h"
#include "XUtil.h"
#include <cassert>

#pragma warning(disable: 26812)

using namespace Microsoft::WRL;

Texture2DBase::Texture2DBase(ID3D11Device* device, 
    const CD3D11_TEXTURE2D_DESC& texDesc, const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc)
    : m_Width(texDesc.Width), m_Height(texDesc.Height)
{
    m_pTexture.Reset();
    m_pTextureSRV.Reset();

    device->CreateTexture2D(&texDesc, nullptr, m_pTexture.GetAddressOf());

    // 用实际产生的mip等级等数据更新
    D3D11_TEXTURE2D_DESC desc;
    m_pTexture->GetDesc(&desc);

    // 完整资源SRV
    if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)) 
    {
        device->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pTextureSRV.GetAddressOf());
    }
}

void Texture2DBase::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pTexture.Get(), name);
    ::SetDebugObjectName(m_pTextureSRV.Get(), std::string(name) + ".SRV");
#endif
}

Texture2D::Texture2D(ID3D11Device* device, uint32_t width, uint32_t height,
    DXGI_FORMAT format, uint32_t mipLevels, uint32_t bindFlags)
    : Texture2DBase(device, CD3D11_TEXTURE2D_DESC(format, width, height, 1, mipLevels, bindFlags), 
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, format))
{
    D3D11_TEXTURE2D_DESC desc;
    m_pTexture->GetDesc(&desc);
    m_MipLevels = desc.MipLevels;
    if (bindFlags & D3D11_BIND_RENDER_TARGET)
    {
        device->CreateRenderTargetView(m_pTexture.Get(), nullptr, m_pTextureRTV.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_TEXTURE2D, format);
        device->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, m_pTextureUAV.GetAddressOf());
    }

}

void Texture2D::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pTextureRTV.Get(), std::string(name) + ".RTV");
#endif
}

Texture2DMS::Texture2DMS(ID3D11Device* device, uint32_t width, uint32_t height, 
    DXGI_FORMAT format, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t bindFlags)
    : Texture2DBase(device, 
        CD3D11_TEXTURE2D_DESC(format, width, height, 1, 1, bindFlags, 
            D3D11_USAGE_DEFAULT, 0, sampleDesc.Count, sampleDesc.Quality), 
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DMS, format)), 
    m_MsaaSamples(sampleDesc.Count)
{
    if (bindFlags & D3D11_BIND_RENDER_TARGET)
    {
        device->CreateRenderTargetView(m_pTexture.Get(), nullptr, m_pTextureRTV.GetAddressOf());
    }
}

void Texture2DMS::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pTextureRTV.Get(), std::string(name) + ".RTV");
#endif
}

TextureCube::TextureCube(ID3D11Device* device, uint32_t width, uint32_t height,
    DXGI_FORMAT format, uint32_t mipLevels, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(format, width, height, 6, mipLevels, bindFlags, D3D11_USAGE_DEFAULT,
            0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURECUBE, format))
{
    D3D11_TEXTURE2D_DESC desc;
    m_pTexture->GetDesc(&desc);
    m_MipLevels = desc.MipLevels;
    if (bindFlags & D3D11_BIND_RENDER_TARGET)
    {
        // 单个子资源
        for (uint32_t i = 0; i < 6; ++i) {
            CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(
                D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11RenderTargetView> pRTV;
            device->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, pRTV.GetAddressOf());
            m_pRenderTargetElements.push_back(pRTV);
        }

        // 完整资源
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, format, 0);
        device->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pTextureArrayRTV.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS) 
    {
        // 单个子资源
        for (uint32_t i = 0; i < 6; ++i) {
            CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(
                D3D11_UAV_DIMENSION_TEXTURE2DARRAY,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11UnorderedAccessView> pUAV;
            device->CreateUnorderedAccessView(m_pTexture.Get(), &uavDesc, pUAV.GetAddressOf());
            m_pUnorderedAccessElements.push_back(pUAV);
        }
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE) 
    {
        // 单个子资源
        for (uint32_t i = 0; i < 6; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
                format,
                0, (uint32_t)-1,
                i, 1   // Array
            );

            ComPtr<ID3D11ShaderResourceView> pSRV;
            device->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }

}

void TextureCube::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    for (size_t i = 0; i < m_pRenderTargetElements.size(); ++i)
        ::SetDebugObjectName(m_pRenderTargetElements[i].Get(), std::string(name) + ".RTV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
        ::SetDebugObjectName(m_pShaderResourceElements[i].Get(), std::string(name) + ".SRV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pUnorderedAccessElements.size(); ++i)
        ::SetDebugObjectName(m_pUnorderedAccessElements[i].Get(), std::string(name) + ".UAV[" + std::to_string(i) + "]");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

Texture2DArray::Texture2DArray(ID3D11Device* device, uint32_t width, uint32_t height,
    DXGI_FORMAT format, uint32_t arraySize, uint32_t mipLevels, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, mipLevels, bindFlags),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DARRAY, format)), 
    m_ArraySize(arraySize)
{
    D3D11_TEXTURE2D_DESC desc;
    m_pTexture->GetDesc(&desc);
    m_MipLevels = desc.MipLevels;
    if (bindFlags & D3D11_BIND_RENDER_TARGET)
    {
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(
                D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11RenderTargetView> pRTV;
            device->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, pRTV.GetAddressOf());
            m_pRenderTargetElements.push_back(pRTV);
        }
    }

    if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
    {
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(
                D3D11_UAV_DIMENSION_TEXTURE2DARRAY,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11UnorderedAccessView> pUAV;
            device->CreateUnorderedAccessView(m_pTexture.Get(), &uavDesc, pUAV.GetAddressOf());
            m_pUnorderedAccessElements.push_back(pUAV);
        }
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
                format,
                0, (uint32_t)-1,
                i, 1   // Array
            );

            ComPtr<ID3D11ShaderResourceView> pSRV;
            device->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}

void Texture2DArray::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    for (size_t i = 0; i < m_pRenderTargetElements.size(); ++i)
        ::SetDebugObjectName(m_pRenderTargetElements[i].Get(), std::string(name) + ".RTV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
        ::SetDebugObjectName(m_pShaderResourceElements[i].Get(), std::string(name) + ".SRV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pUnorderedAccessElements.size(); ++i)
        ::SetDebugObjectName(m_pUnorderedAccessElements[i].Get(), std::string(name) + ".UAV[" + std::to_string(i) + "]");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

Texture2DMSArray::Texture2DMSArray(ID3D11Device* device, uint32_t width, uint32_t height,
    DXGI_FORMAT format, uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(format, width, height, arraySize, 1, bindFlags, 
            D3D11_USAGE_DEFAULT, 0, sampleDesc.Count, sampleDesc.Quality),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, format)), 
    m_ArraySize(arraySize), m_MsaaSamples(sampleDesc.Count)
{
    if (bindFlags & D3D11_BIND_RENDER_TARGET)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(
                D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY,
                format,
                0,          // Mips
                i, 1        // Array
            );

            ComPtr<ID3D11RenderTargetView> pRTV;
            device->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, pRTV.GetAddressOf());
            m_pRenderTargetElements.push_back(pRTV);
        }

        // 完整资源
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY, format, 0);
        device->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pTextureArrayRTV.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY,
                format,
                0, (uint32_t)-1,
                i, 1   // Array
            );

            ComPtr<ID3D11ShaderResourceView> pSRV;
            device->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}

void Texture2DMSArray::SetDebugObjectName(std::string_view name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    for (size_t i = 0; i < m_pRenderTargetElements.size(); ++i)
        ::SetDebugObjectName(m_pRenderTargetElements[i].Get(), std::string(name) + ".RTV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
        ::SetDebugObjectName(m_pShaderResourceElements[i].Get(), std::string(name) + ".SRV[" + std::to_string(i) + "]");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

static DXGI_FORMAT GetDepthTextureFormat(DepthStencilBitsFlag flag)
{
    switch (flag)
    {
    case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_R16_TYPELESS;
    case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_R24G8_TYPELESS;
    case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_R32_TYPELESS;
    case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_R32G8X24_TYPELESS;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

static DXGI_FORMAT GetDepthSRVFormat(DepthStencilBitsFlag flag)
{
    switch (flag)
    {
    case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_R16_FLOAT;
    case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_R32_FLOAT;
    case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

static DXGI_FORMAT GetDepthDSVFormat(DepthStencilBitsFlag flag)
{
    switch (flag)
    {
    case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_D16_UNORM;
    case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_D32_FLOAT;
    case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
    default: return DXGI_FORMAT_UNKNOWN;
    }
}

Depth2D::Depth2D(ID3D11Device* device, 
    uint32_t width, uint32_t height, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(GetDepthTextureFormat(depthStencilBitsFlag), width, height, 1, 1, bindFlags),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, GetDepthSRVFormat(depthStencilBitsFlag)))
{
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2D, GetDepthDSVFormat(depthStencilBitsFlag));
        device->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pTextureDSV.GetAddressOf());
    }
}

void Depth2D::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pTextureDSV.Get(), std::string(name) + ".DSV");
#endif
}

Depth2DMS::Depth2DMS(ID3D11Device* device, uint32_t width, uint32_t height,
    const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(GetDepthTextureFormat(depthStencilBitsFlag), width, height, 1, 1, bindFlags, 
            D3D11_USAGE_DEFAULT, 0, sampleDesc.Count, sampleDesc.Quality),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DMS, GetDepthSRVFormat(depthStencilBitsFlag))), 
    m_MsaaSamples(sampleDesc.Count)
{
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS, GetDepthDSVFormat(depthStencilBitsFlag));
        device->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pTextureDSV.GetAddressOf());
    }
}

void Depth2DMS::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pTextureDSV.Get(), std::string(name) + ".DSV");
#endif
}

Depth2DArray::Depth2DArray(ID3D11Device* device, uint32_t width, uint32_t height,
    uint32_t arraySize, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(GetDepthTextureFormat(depthStencilBitsFlag), width, height, arraySize, 1, bindFlags),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DARRAY, GetDepthSRVFormat(depthStencilBitsFlag))), 
    m_ArraySize(arraySize)
{
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_DEPTH_STENCIL_VIEW_DESC dsvElementDesc(
                D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
                GetDepthDSVFormat(depthStencilBitsFlag),
                0, i, 1);   // Array

            ComPtr<ID3D11DepthStencilView> pDSV;
            device->CreateDepthStencilView(m_pTexture.Get(), &dsvElementDesc, pDSV.GetAddressOf());
            m_pDepthStencilElements.push_back(pDSV);
        }

        // 完整子资源
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
            D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
            GetDepthDSVFormat(depthStencilBitsFlag), 0);
        device->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDepthArrayDSV.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
                GetDepthSRVFormat(depthStencilBitsFlag),
                0, 1, 
                i, 1);   // Array

            ComPtr<ID3D11ShaderResourceView> pSRV;
            device->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}

void Depth2DArray::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    for (size_t i = 0; i < m_pDepthStencilElements.size(); ++i)
        ::SetDebugObjectName(m_pDepthStencilElements[i].Get(), std::string(name) + ".DSV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
        ::SetDebugObjectName(m_pShaderResourceElements[i].Get(), std::string(name) + ".SRV[" + std::to_string(i) + "]");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

Depth2DMSArray::Depth2DMSArray(ID3D11Device* device, uint32_t width, uint32_t height, uint32_t arraySize, 
    const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
    : Texture2DBase(device,
        CD3D11_TEXTURE2D_DESC(GetDepthTextureFormat(depthStencilBitsFlag), width, height, arraySize, 1, bindFlags,
            D3D11_USAGE_DEFAULT, 0, sampleDesc.Count, sampleDesc.Quality),
        CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY, GetDepthSRVFormat(depthStencilBitsFlag))), 
    m_ArraySize(arraySize), m_MsaaSamples(sampleDesc.Count)
{
    if (bindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_DEPTH_STENCIL_VIEW_DESC dsvElementDesc(
                D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY,
                GetDepthDSVFormat(depthStencilBitsFlag),
                0, i, 1);   // Array

            ComPtr<ID3D11DepthStencilView> pDSV;
            device->CreateDepthStencilView(m_pTexture.Get(), &dsvElementDesc, pDSV.GetAddressOf());
            m_pDepthStencilElements.push_back(pDSV);
        }

        // 完整子资源
        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
            D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY,
            GetDepthDSVFormat(depthStencilBitsFlag), 0);
        device->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDepthArrayDSV.GetAddressOf());
    }

    if (bindFlags & D3D11_BIND_SHADER_RESOURCE)
    {
        // 单个子资源
        for (uint32_t i = 0; i < arraySize; ++i) {
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvElementDesc(
                D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY,
                GetDepthSRVFormat(depthStencilBitsFlag),
                0, 1,
                i, 1);   // Array

            ComPtr<ID3D11ShaderResourceView> pSRV;
            device->CreateShaderResourceView(m_pTexture.Get(), &srvElementDesc, pSRV.GetAddressOf());
            m_pShaderResourceElements.push_back(pSRV);
        }
    }
}

void Depth2DMSArray::SetDebugObjectName(std::string_view name)
{
    Texture2DBase::SetDebugObjectName(name);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    for (size_t i = 0; i < m_pDepthStencilElements.size(); ++i)
        ::SetDebugObjectName(m_pDepthStencilElements[i].Get(), std::string(name) + ".DSV[" + std::to_string(i) + "]");
    for (size_t i = 0; i < m_pShaderResourceElements.size(); ++i)
        ::SetDebugObjectName(m_pShaderResourceElements[i].Get(), std::string(name) + ".SRV[" + std::to_string(i) + "]");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}

