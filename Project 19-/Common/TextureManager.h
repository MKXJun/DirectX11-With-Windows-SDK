//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 管理纹理资源
// Simple game object.
//***************************************************************************************

#pragma once

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H


#include <unordered_map>
#include <string>
#include "WinMin.h"
#include <d3d11_1.h>
#include <wrl/client.h>
#include <XUtil.h>

class TextureManager
{
public:
    enum LoadConfig : uint32_t
    {
        LoadConfig_EnableMips = 0x1,
        LoadConfig_ForceSRGB = 0x2,
        LoadConfig_HDR_Float16 = 0x4
    };

public:
    TextureManager();
    ~TextureManager();
    TextureManager(TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = default;
    TextureManager& operator=(TextureManager&&) = default;

    static TextureManager& Get();
    void Init(ID3D11Device* device);
    ID3D11ShaderResourceView* CreateFromFile(std::string_view filename, uint32_t loadConfig = 0);
    ID3D11ShaderResourceView* CreateFromMemory(std::string_view name, const void* data, size_t byteWidth, uint32_t loadConfig = 0);
    bool AddTexture(std::string_view name, ID3D11ShaderResourceView* texture);
    void RemoveTexture(std::string_view name);

    // 特殊名称: 
    //   $Null/$Ones: 1x1的(1, 1, 1, 1)纹理
    //   $Zeros:      1x1的(0, 0, 0, 0)纹理
    //   $Normal:     1x1的纹理解压后为(0, 0, 1)法线
    //   $NullARM:    1x1的(1, 1, 0, 0)纹理
    ID3D11ShaderResourceView* GetTexture(std::string_view filename);

private:
    ID3D11ShaderResourceView* CreateHDRTexture(const float* data, int width, int height, int comp, uint32_t loadConfig);
    ID3D11ShaderResourceView* CreateLDRTexture(const uint8_t* data, int width, int height, int comp, uint32_t loadConfig);

private:

    Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
    std::unordered_map<XID, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_TextureSRVs;
};

#endif
