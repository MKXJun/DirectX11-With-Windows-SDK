#define STBI_WINDOWS_UTF8
#include "stb_image.h"
#include "TextureManager.h"
#include "DXTrace.h"
#include "DDSTextureLoader.h"

namespace
{
	// ForwardEffect单例
	static TextureManager* g_pInstance = nullptr;
}

TextureManager::TextureManager()
{
	if (g_pInstance)
		throw std::exception("ForwardEffect is a singleton!");
	g_pInstance = this;
}

TextureManager::~TextureManager()
{
}

TextureManager& TextureManager::Get()
{
	if (!g_pInstance)
		throw std::exception("TextureManager needs an instance!");
	return *g_pInstance;
}

void TextureManager::Init(ID3D11Device* device)
{
	m_pDevice = device;
	m_pDevice->GetImmediateContext(m_pDeviceContext.ReleaseAndGetAddressOf());
}

ID3D11ShaderResourceView* TextureManager::CreateTexture(const std::wstring& filename, bool enableMips, bool forceSRGB)
{
	if (m_TextureSRVs.count(filename))
		return m_TextureSRVs[filename].Get();

	auto& res = m_TextureSRVs[filename];
	
	if (FAILED(DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(),
		enableMips ? m_pDeviceContext.Get() : nullptr,
		filename.c_str(), 0, D3D11_USAGE_DEFAULT,
		D3D11_BIND_SHADER_RESOURCE | (enableMips ? D3D11_BIND_RENDER_TARGET : 0), 0,
		(enableMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0), 
		forceSRGB, nullptr, res.GetAddressOf())))
	{
		int width, height, comp;
		char filename_utf8[500];
		stbi_convert_wchar_to_utf8(filename_utf8, 500, filename.c_str());
		stbi_uc* img_data = stbi_load(filename_utf8, &width, &height, &comp, STBI_rgb_alpha);
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
			m_pDeviceContext->UpdateSubresource(tex.Get(), 0, nullptr, img_data, width * sizeof(uint32_t), 0);
			CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, 
				forceSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM);
			HR(m_pDevice->CreateShaderResourceView(tex.Get(), &srvDesc, res.GetAddressOf()));
			if (enableMips)
				m_pDeviceContext->GenerateMips(res.Get());
		}
		stbi_image_free(img_data);
	}

	return res.Get();
}
