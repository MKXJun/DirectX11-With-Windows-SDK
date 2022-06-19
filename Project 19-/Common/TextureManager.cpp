#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "TextureManager.h"
#include "XUtil.h"
#include "DXTrace.h"
#include <DDSTextureLoader11.h>
#include <filesystem>

using namespace Microsoft::WRL;

namespace
{
    // TextureManager单例
    TextureManager* s_pInstance = nullptr;
}

TextureManager::TextureManager()
{
    if (s_pInstance)
        throw std::exception("TextureManager is a singleton!");
    s_pInstance = this;
}

TextureManager::~TextureManager()
{
}

TextureManager& TextureManager::Get()
{
    if (!s_pInstance)
        throw std::exception("TextureManager needs an instance!");
    return *s_pInstance;
}

void TextureManager::Init(ID3D11Device* device)
{
    m_pDevice = device;
    m_pDevice->GetImmediateContext(m_pDeviceContext.ReleaseAndGetAddressOf());
}

ID3D11ShaderResourceView* TextureManager::CreateTexture(std::string_view filename, bool enableMips, bool forceSRGB)
{
    XID fileID = StringToID(filename);
    if (m_TextureSRVs.count(fileID))
        return m_TextureSRVs[fileID].Get();

    auto& res = m_TextureSRVs[fileID];
    ComPtr<ID3D11Texture2D> pTex;
    std::wstring wstr = UTF8ToWString(filename);
    if (FAILED(DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(),
        enableMips ? m_pDeviceContext.Get() : nullptr,
        wstr.c_str(), 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, 0, 
        forceSRGB, nullptr, res.ReleaseAndGetAddressOf())))
    {
        int width, height, comp;
        
        stbi_uc* img_data = stbi_load(filename.data(), &width, &height, &comp, STBI_rgb_alpha);
        if (img_data)
        {
            CD3D11_TEXTURE2D_DESC texDesc(forceSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM, 
                width, height, 1,
                enableMips ? 0 : 1, 
                D3D11_BIND_SHADER_RESOURCE | (enableMips ? D3D11_BIND_RENDER_TARGET : 0),
                D3D11_USAGE_DEFAULT, 0, 1, 0, 
                enableMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);
            Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
            HR(m_pDevice->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf()));
            // 上传纹理数据
            m_pDeviceContext->UpdateSubresource(tex.Get(), 0, nullptr, img_data, width * sizeof(uint32_t), 0);
            CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, 
                forceSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
            // 创建SRV
            HR(m_pDevice->CreateShaderResourceView(tex.Get(), &srvDesc, res.ReleaseAndGetAddressOf()));
            // 生成mipmap
            if (enableMips)
                m_pDeviceContext->GenerateMips(res.Get());
            stbi_image_free(img_data);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
            SetDebugObjectName(res.Get(), std::filesystem::path(filename).filename().string());
#endif
        }
        else
        {
            std::string warning = "[Warning]: TextureManager::CreateTexture, couldn't find \"";
            warning += filename;
            warning += "\"\n";
            OutputDebugStringA(warning.c_str());
        }
    }

    return res.Get();
}

bool TextureManager::AddTexture(std::string_view name, ID3D11ShaderResourceView* texture)
{
    XID nameID = StringToID(name);
    return m_TextureSRVs.try_emplace(nameID, texture).second;
}

void TextureManager::RemoveTexture(std::string_view name)
{
    XID nameID = StringToID(name);
    m_TextureSRVs.erase(nameID);
}

ID3D11ShaderResourceView* TextureManager::GetTexture(std::string_view filename)
{
    XID fileID = StringToID(filename);
    if (m_TextureSRVs.count(fileID))
        return m_TextureSRVs[fileID].Get();
    return nullptr;
}
