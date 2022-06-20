#pragma once

#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "WinMin.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <string_view>

class Texture2DBase
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    Texture2DBase(ID3D11Device* device, const CD3D11_TEXTURE2D_DESC& texDesc, const CD3D11_SHADER_RESOURCE_VIEW_DESC& srvDesc);
    virtual ~Texture2DBase() = 0 {}

    ID3D11Texture2D* GetTexture() { return m_pTexture.Get(); }
    // 获取访问完整资源的视图
    ID3D11ShaderResourceView* GetShaderResource() { return m_pTextureSRV.Get(); }

    uint32_t GetWidth() const { return m_Width; }
    uint32_t GetHeight() const { return m_Height; }

    // 设置调试对象名
    virtual void SetDebugObjectName(std::string_view name);

protected:
    ComPtr<ID3D11Texture2D> m_pTexture;
    ComPtr<ID3D11ShaderResourceView> m_pTextureSRV;
    uint32_t m_Width{}, m_Height{};
};

class Texture2D : public Texture2DBase
{
public:
    Texture2D(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
        uint32_t mipLevels = 1,
        uint32_t bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    ~Texture2D() override = default;

    // 不允许拷贝，允许移动
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) = default;
    Texture2D& operator=(Texture2D&&) = default;

    ID3D11RenderTargetView* GetRenderTarget() { return m_pTextureRTV.Get(); }
    ID3D11UnorderedAccessView* GetUnorderedAccess() { return m_pTextureUAV.Get(); }

    uint32_t GetMipLevels() const { return m_MipLevels; }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

protected:
    uint32_t m_MipLevels = 1;
    ComPtr<ID3D11RenderTargetView> m_pTextureRTV;
    ComPtr<ID3D11UnorderedAccessView> m_pTextureUAV;
};

class Texture2DMS : public Texture2DBase
{
public:
    Texture2DMS(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
        const DXGI_SAMPLE_DESC& sampleDesc, 
        uint32_t bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);
    ~Texture2DMS() override = default;

    ID3D11RenderTargetView* GetRenderTarget() { return m_pTextureRTV.Get(); }

    uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_MsaaSamples = 1;
    ComPtr<ID3D11RenderTargetView> m_pTextureRTV;
};

class TextureCube : public Texture2DBase
{
public:
    TextureCube(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
        uint32_t mipLevels = 1,
        uint32_t bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    ~TextureCube() override = default;

    uint32_t GetMipLevels() const { return m_MipLevels; }

    ID3D11RenderTargetView* GetRenderTarget() { return m_pTextureArrayRTV.Get(); }
    ID3D11RenderTargetView* GetRenderTarget(size_t arrayIdx) { return m_pRenderTargetElements[arrayIdx].Get(); }
    
    // RWTexture2D
    ID3D11UnorderedAccessView* GetUnorderedAccess(size_t arrayIdx) { return m_pUnorderedAccessElements[arrayIdx].Get(); }
    
    // TextureCube
    using Texture2DBase::GetShaderResource;
    // Texture2D
    ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_MipLevels = 1;
    ComPtr<ID3D11RenderTargetView> m_pTextureArrayRTV;   // RTV指向纹理数组
    std::vector<ComPtr<ID3D11RenderTargetView>> m_pRenderTargetElements;
    std::vector<ComPtr<ID3D11UnorderedAccessView>> m_pUnorderedAccessElements;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;
};

class Texture2DArray : public Texture2DBase
{
public:
    Texture2DArray(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
        uint32_t arraySize, uint32_t mipLevels = 1,
        uint32_t bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    ~Texture2DArray() override = default;

    uint32_t GetMipLevels() const { return m_MipLevels; }
    uint32_t GetArraySize() const { return m_ArraySize; }

    ID3D11RenderTargetView* GetRenderTarget() { return m_pTextureArrayRTV.Get(); }
    ID3D11RenderTargetView* GetRenderTarget(size_t arrayIdx) { return m_pRenderTargetElements[arrayIdx].Get(); }
    
    // RWTexture2D
    ID3D11UnorderedAccessView* GetUnorderedAccess(size_t arrayIdx) { return m_pUnorderedAccessElements[arrayIdx].Get(); }

    // Texture2DArray
    using Texture2DBase::GetShaderResource;
    // Texture2D
    ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_MipLevels = 1;
    uint32_t m_ArraySize = 1;
    ComPtr<ID3D11RenderTargetView> m_pTextureArrayRTV;   // RTV指向纹理数组
    std::vector<ComPtr<ID3D11RenderTargetView>> m_pRenderTargetElements;
    std::vector<ComPtr<ID3D11UnorderedAccessView>> m_pUnorderedAccessElements;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;
};

class Texture2DMSArray : public Texture2DBase
{
public:
    Texture2DMSArray(ID3D11Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
        uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc,
        uint32_t bindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    ~Texture2DMSArray() override = default;
    
    uint32_t GetArraySize() const { return m_ArraySize; }
    uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

    ID3D11RenderTargetView* GetRenderTarget() { return m_pTextureArrayRTV.Get(); }
    ID3D11RenderTargetView* GetRenderTarget(size_t arrayIdx) { return m_pRenderTargetElements[arrayIdx].Get(); }

    // Texture2DMSArray
    using Texture2DBase::GetShaderResource;
    // Texture2DMS
    ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_MsaaSamples = 1;
    uint32_t m_ArraySize = 1;

    ComPtr<ID3D11RenderTargetView> m_pTextureArrayRTV;   // RTV指向纹理数组
    std::vector<ComPtr<ID3D11RenderTargetView>> m_pRenderTargetElements;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;
};

enum class DepthStencilBitsFlag
{
    Depth_16Bits = 0,
    Depth_24Bits_Stencil_8Bits = 1,
    Depth_32Bits = 2,
    Depth_32Bits_Stencil_8Bits_Unused_24Bits = 3,
};


class Depth2D : public Texture2DBase
{
public:
    Depth2D(ID3D11Device* device, uint32_t width, uint32_t height,
        DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
        uint32_t bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
    ~Depth2D() override = default;

    ID3D11DepthStencilView* GetDepthStencil() { return m_pTextureDSV.Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    ComPtr<ID3D11DepthStencilView> m_pTextureDSV;
};

class Depth2DMS : public Texture2DBase
{
public:
    Depth2DMS(ID3D11Device* device, uint32_t width, uint32_t height,
        const DXGI_SAMPLE_DESC& sampleDesc,
        DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
        uint32_t bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
    ~Depth2DMS() override = default;

    uint32_t GetMsaaSamples() const { return m_MsaaSamples; }
    ID3D11DepthStencilView* GetDepthStencil() { return m_pTextureDSV.Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_MsaaSamples = 1;
    ComPtr<ID3D11DepthStencilView> m_pTextureDSV;
};

class Depth2DArray : public Texture2DBase
{
public:
    Depth2DArray(ID3D11Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
        DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
        uint32_t bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
    ~Depth2DArray() override = default;

    ID3D11DepthStencilView* GetDepthStencil() { return m_pDepthArrayDSV.Get(); }
    ID3D11DepthStencilView* GetDepthStencil(size_t arrayIdx) { return m_pDepthStencilElements[arrayIdx].Get(); }

    // TextureArray
    using Texture2DBase::GetShaderResource;
    // Texture2D
    ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

    uint32_t GetArraySize() const { return m_ArraySize; }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_ArraySize = 1;
    ComPtr<ID3D11DepthStencilView> m_pDepthArrayDSV;
    std::vector<ComPtr<ID3D11DepthStencilView>> m_pDepthStencilElements;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;
};

class Depth2DMSArray : public Texture2DBase
{
public:
    Depth2DMSArray(ID3D11Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
        const DXGI_SAMPLE_DESC& sampleDesc,
        DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
        uint32_t bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);
    ~Depth2DMSArray() override = default;

    uint32_t GetArraySize() const { return m_ArraySize; }
    uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

    ID3D11DepthStencilView* GetDepthStencil() { return m_pDepthArrayDSV.Get(); }
    ID3D11DepthStencilView* GetDepthStencil(size_t arrayIdx) { return m_pDepthStencilElements[arrayIdx].Get(); }

    // Texture2DMSArray
    using Texture2DBase::GetShaderResource;
    // Texture2DMS
    ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

    // 设置调试对象名
    void SetDebugObjectName(std::string_view name) override;

private:
    uint32_t m_ArraySize = 1;
    uint32_t m_MsaaSamples = 1;
    ComPtr<ID3D11DepthStencilView> m_pDepthArrayDSV;
    std::vector<ComPtr<ID3D11DepthStencilView>> m_pDepthStencilElements;
    std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;
};


#endif
