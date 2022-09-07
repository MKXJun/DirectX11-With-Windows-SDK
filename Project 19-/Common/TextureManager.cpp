#define STB_IMAGE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 26451)
#pragma warning(disable: 26819)
#pragma warning(disable: 6262)
#include "stb_image.h"
#pragma warning(pop)

#include "TextureManager.h"
#include "XUtil.h"
#include "DXTrace.h"
#include "ImGuiLog.h"
#include <DDSTextureLoader11.h>
#include <filesystem>
#include <DirectXPackedVector.h>

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

    // 加一张1x1的白颜色
    ComPtr<ID3D11Texture2D> pTex;
    CD3D11_TEXTURE2D_DESC nullTexDesc(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1);
    D3D11_SUBRESOURCE_DATA initData{};
    uint32_t white = (uint32_t)-1;
    initData.pSysMem = &white;
    initData.SysMemPitch = 4;
    m_pDevice->CreateTexture2D(&nullTexDesc, &initData, &pTex);

    ComPtr<ID3D11ShaderResourceView> pSRV;
    m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, &pSRV);
    m_TextureSRVs.try_emplace(StringToID("$Null"), pSRV.Get());
    m_TextureSRVs.try_emplace(StringToID("$Ones"), pSRV.Get());

    // 一张1x1的黑颜色
    uint32_t black = 0;
    initData.pSysMem = &black;
    m_pDevice->CreateTexture2D(&nullTexDesc, &initData, &pTex);
    m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, &pSRV);
    m_TextureSRVs.try_emplace(StringToID("$Zeros"), pSRV.Get());

    // 一张1x1的默认法线
    uint32_t normal = 0x00FF8080;
    initData.pSysMem = &normal;
    m_pDevice->CreateTexture2D(&nullTexDesc, &initData, &pTex);
    m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, &pSRV);
    m_TextureSRVs.try_emplace(StringToID("$Normal"), pSRV.Get());

    // 一张1x1的默认Roughness_Metallic
    uint32_t arm = 0x0000FF00;
    initData.pSysMem = &arm;
    m_pDevice->CreateTexture2D(&nullTexDesc, &initData, &pTex);
    m_pDevice->CreateShaderResourceView(pTex.Get(), nullptr, &pSRV);
    m_TextureSRVs.try_emplace(StringToID("$NullRoughnessMetallic"), pSRV.Get());
}

ID3D11ShaderResourceView* TextureManager::CreateFromFile(std::string_view filename, uint32_t loadConfig)
{
    XID fileID = StringToID(filename);
    if (m_TextureSRVs.count(fileID))
        return m_TextureSRVs[fileID].Get();

    auto& res = m_TextureSRVs[fileID];
    ComPtr<ID3D11Texture2D> pTex;
    std::wstring wstr = UTF8ToWString(filename);
    if (FAILED(DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(),
        loadConfig & LoadConfig_EnableMips ? m_pDeviceContext.Get() : nullptr,
        wstr.c_str(), 0, D3D11_USAGE_DEFAULT,
        D3D11_BIND_SHADER_RESOURCE, 0, 0,
        !!(loadConfig & LoadConfig_ForceSRGB),
        nullptr, res.ReleaseAndGetAddressOf())))
    {
        namespace fs = std::filesystem;
        int width, height, comp;

        FILE* f = stbi__fopen(filename.data(), "rb");
        if (f == nullptr)
        {
            std::string warning = "[Warning]: TextureManager::CreateFromFile, couldn't find \"";
            warning += filename;
            warning += "\"\n";

            if (ImGuiLog::HasInstance())
            {
                ImGuiLog::Get().AddLog(warning.c_str());
            }
            else
            {
                OutputDebugStringA(warning.c_str());
            }
            return nullptr;
        }

        // 预先判定通道数
        if (!stbi_info_from_file(f, &width, &height, &comp))
        {
            OutputDebugStringA(stbi_failure_reason());
            return nullptr;
        }

        // HDR Image
        if (stbi_is_hdr_from_file(f))
        {
            float* img_data = stbi_loadf_from_file(f, &width, &height, &comp, comp >= 3 ? STBI_rgb_alpha : STBI_default);
            if (comp >= 3) comp = 4;
            res.Attach(CreateHDRTexture(img_data, width, height, comp, loadConfig));
            stbi_image_free(img_data);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
            SetDebugObjectName(res.Get(), std::filesystem::path(filename).filename().string());
#endif
        }
        // LDR Image
        else
        {
            stbi_uc* img_data = stbi_load(filename.data(), &width, &height, &comp, comp >= 3 ? STBI_rgb_alpha : STBI_default);
            if (comp >= 3) comp = 4;
            res.Attach(CreateLDRTexture(img_data, width, height, comp, loadConfig));
            stbi_image_free(img_data);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
            SetDebugObjectName(res.Get(), std::filesystem::path(filename).filename().string());
#endif
        }
    }

    return res.Get();
}

ID3D11ShaderResourceView* TextureManager::CreateFromMemory(std::string_view name, const void* data, size_t byteWidth, uint32_t loadConfig)
{
    XID fileID = StringToID(name);
    if (m_TextureSRVs.count(fileID))
        return m_TextureSRVs[fileID].Get();

    auto& res = m_TextureSRVs[fileID];
    int width, height, comp;

    // 预先判定通道数
    const stbi_uc* buffer = reinterpret_cast<const stbi_uc*>(data);
    if (!stbi_info_from_memory(buffer, (int)byteWidth, &width, &height, &comp))
    {
        OutputDebugStringA(stbi_failure_reason());
        return nullptr;
    }

    // HDR image
    if (stbi_is_hdr_from_memory(buffer, static_cast<int>(byteWidth)))
    {
        float* img_data = stbi_loadf_from_memory(buffer, (int)byteWidth, &width, &height, &comp, comp >= 3 ? STBI_rgb_alpha : STBI_default);
        if (comp >= 3) comp = 4;
        res.Attach(CreateHDRTexture(img_data, width, height, comp, loadConfig));
        stbi_image_free(img_data);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
        SetDebugObjectName(res.Get(), name);
#endif
    }
    // LDR Image
    else
    {
        stbi_uc* img_data = stbi_load_from_memory(buffer, (int)byteWidth, &width, &height, &comp, STBI_rgb_alpha);
        if (comp >= 3) comp = 4;
        res.Attach(CreateLDRTexture(img_data, width, height, comp, loadConfig));
        stbi_image_free(img_data);
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
        SetDebugObjectName(res.Get(), name.data());
#endif
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

ID3D11ShaderResourceView* TextureManager::CreateHDRTexture(const float* data, int width, int height, int comp, uint32_t loadConfig)
{
    DXGI_FORMAT resFormat{};
    if (comp == 1) resFormat = DXGI_FORMAT_R32_FLOAT;
    else if (comp == 2) resFormat = DXGI_FORMAT_R32G32_FLOAT;
    else if (comp == 3) resFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    else if (comp == 4) resFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;

    CD3D11_TEXTURE2D_DESC texDesc(
        resFormat, width, height, 1,
        loadConfig & LoadConfig_EnableMips ? 0 : 1,
        D3D11_BIND_SHADER_RESOURCE | (loadConfig & LoadConfig_EnableMips ? D3D11_BIND_RENDER_TARGET : 0),
        D3D11_USAGE_DEFAULT, 0, 1, 0,
        loadConfig & LoadConfig_EnableMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HR(m_pDevice->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf()));
    // 上传纹理数据
    if (loadConfig & LoadConfig_HDR_Float16)
    {
        std::vector<DirectX::PackedVector::HALF> initData(width * height * comp);
        DirectX::PackedVector::XMConvertFloatToHalfStream(initData.data(), 2, data, 4, width * height * comp);
        m_pDeviceContext->UpdateSubresource(tex.Get(), 0, nullptr, initData.data(), width * sizeof(uint16_t), 0);
    }
    else
    {
        m_pDeviceContext->UpdateSubresource(tex.Get(), 0, nullptr, data, width * sizeof(float) * comp, 0);
    }

    // 创建SRV
    ID3D11ShaderResourceView* pSRV = nullptr;
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, resFormat);
    HR(m_pDevice->CreateShaderResourceView(tex.Get(), &srvDesc, &pSRV));

    // 生成mipmap
    if (loadConfig & LoadConfig_EnableMips)
        m_pDeviceContext->GenerateMips(pSRV);

    return pSRV;
}

ID3D11ShaderResourceView* TextureManager::CreateLDRTexture(const uint8_t* data, int width, int height, int comp, uint32_t loadConfig)
{
    DXGI_FORMAT resFormat{};
    if (comp == 1) resFormat = DXGI_FORMAT_R8_UNORM;
    else if (comp == 2) resFormat = DXGI_FORMAT_R8G8_UNORM;
    else resFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;

    DXGI_FORMAT srvFormat{};
    if (comp == 1) srvFormat = DXGI_FORMAT_R8_UNORM;
    else if (comp == 2) srvFormat = DXGI_FORMAT_R8G8_UNORM;
    else srvFormat = (loadConfig & LoadConfig_ForceSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);

    CD3D11_TEXTURE2D_DESC texDesc(
        resFormat, width, height, 1,
        loadConfig & LoadConfig_EnableMips ? 0 : 1,
        D3D11_BIND_SHADER_RESOURCE | (loadConfig & LoadConfig_EnableMips ? D3D11_BIND_RENDER_TARGET : 0),
        D3D11_USAGE_DEFAULT, 0, 1, 0,
        loadConfig & LoadConfig_EnableMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0);
    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HR(m_pDevice->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf()));
    // 上传纹理数据
    m_pDeviceContext->UpdateSubresource(tex.Get(), 0, nullptr, data, width * comp, 0);
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, srvFormat);
    // 创建SRV
    ID3D11ShaderResourceView* pSRV;
    HR(m_pDevice->CreateShaderResourceView(tex.Get(), &srvDesc, &pSRV));
    // 生成mipmap
    if (loadConfig & LoadConfig_EnableMips)
        m_pDeviceContext->GenerateMips(pSRV);

    return pSRV;
}
