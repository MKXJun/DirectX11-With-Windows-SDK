#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <vector>
#include <string>

class Texture2D
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// Texture2D
	Texture2D(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format, 
		UINT bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
		UINT mipLevels = 1);

	// Texture2DMS
	Texture2D(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format,
		UINT bindFlags, const DXGI_SAMPLE_DESC& sampleDesc);

	// Texture2DArray
	Texture2D(ID3D11Device* d3dDevice, UINT width, UINT height, DXGI_FORMAT format,
		UINT bindFlags, UINT mipLevels, UINT arraySize);

	// Texture2DMSArray
	Texture2D(ID3D11Device* d3dDevice, UINT width, UINT height, DXGI_FORMAT format,
		UINT bindFlags, UINT arraySize, const DXGI_SAMPLE_DESC& sampleDesc);


	~Texture2D() = default;

	// 不允许拷贝，允许移动
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;
	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;

	ID3D11Texture2D* GetTexture() { return m_pTexture.Get(); }
	ID3D11RenderTargetView* GetRenderTarget(size_t arrayIdx = 0) { return m_pRenderTargetElements[arrayIdx].Get(); }
	ID3D11UnorderedAccessView* GetUnorderedAccres(size_t arrayIdx = 0) { return m_pUnorderedAccessElements[arrayIdx].Get(); }
	// 获取访问完整资源的视图
	ID3D11ShaderResourceView* GetShaderResource() { return m_pTextureSRV.Get(); }
	ID3D11ShaderResourceView* GetShaderResource(size_t arrayIdx) { return m_pShaderResourceElements[arrayIdx].Get(); }

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	void InternalConstruct(ID3D11Device* d3dDevice,
		UINT width, UINT height, DXGI_FORMAT format,
		UINT bindFlags, UINT mipLevels, UINT arraySize,
		UINT sampleCount, UINT sampleQuality,
		D3D11_RTV_DIMENSION rtvDimension,
		D3D11_UAV_DIMENSION uavDimension,
		D3D11_SRV_DIMENSION srvDimension);

	ComPtr<ID3D11Texture2D> m_pTexture;
	ComPtr<ID3D11ShaderResourceView> m_pTextureSRV;

	std::vector<ComPtr<ID3D11RenderTargetView>> m_pRenderTargetElements;
	std::vector<ComPtr<ID3D11UnorderedAccessView>> m_pUnorderedAccessElements;
	std::vector<ComPtr<ID3D11ShaderResourceView>> m_pShaderResourceElements;

};

// 当前以32位float作为深度时效果最好
// 可选添加8位模板，但SRVs只会引用32位浮点部分
class Depth2D
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// Texture2D深度缓冲区
	Depth2D(ID3D11Device* d3dDevice, UINT width, UINT height,
		UINT bindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,
		bool stencil = false);

	// Texture2DMS深度缓冲区
	Depth2D(ID3D11Device* d3dDevice, UINT width, UINT height,
		UINT bindFlags, const DXGI_SAMPLE_DESC& sampleDesc,
		bool stencil = false);

	// Texture2DArray深度缓冲区
	Depth2D(ID3D11Device* d3dDevice, UINT width, UINT height,
		UINT bindFlags, UINT arraySize,
		bool stencil = false);

	// Texture2DMSArray深度缓冲区
	Depth2D(ID3D11Device* d3dDevice, UINT width, UINT height,
		UINT bindFlags, UINT arraySize, const DXGI_SAMPLE_DESC& sampleDesc,
		bool stencil = false);

	~Depth2D() = default;

	Depth2D(const Depth2D&) = delete;
	Depth2D& operator=(const Depth2D&) = delete;
	Depth2D(Depth2D&&) = default;
	Depth2D& operator=(Depth2D&&) = default;

	ID3D11Texture2D* GetTexture() { return m_pTexture.Get(); }
	ID3D11DepthStencilView* GetDepthStencil(std::size_t arrayIndex = 0) { return m_pDepthStencilElements[arrayIndex].Get(); }
	ID3D11ShaderResourceView* GetShaderResource() { return m_pTextureSRV.Get(); }

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	void InternalConstruct(ID3D11Device* d3dDevice,
		UINT width, UINT height,
		UINT bindFlags, UINT arraySize,
		UINT sampleCount, UINT sampleQuality,
		D3D11_DSV_DIMENSION dsvDimension,
		D3D11_SRV_DIMENSION srvDimension,
		bool stencil);


	ComPtr<ID3D11Texture2D> m_pTexture;
	ComPtr<ID3D11ShaderResourceView> m_pTextureSRV;
	// 每个元素一个深度/模板视图
	std::vector<ComPtr<ID3D11DepthStencilView>> m_pDepthStencilElements;
};


#endif
