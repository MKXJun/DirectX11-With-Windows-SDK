#pragma once

#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <unordered_map>

#include <string>
#include <d3d11.h>
#include <wrl/client.h>

class TextureManager
{
public:
	TextureManager();
	~TextureManager();
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;
	TextureManager(TextureManager&&) = default;
	TextureManager& operator=(TextureManager&&) = default;

	static TextureManager& Get();
	void Init(ID3D11Device* device);
	ID3D11ShaderResourceView* CreateTexture(std::string_view filename, bool enableMips = false, bool forceSRGB = false);
	ID3D11ShaderResourceView* GetTexture(std::string_view filename);

private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_TextureSRVs;
};

#endif
