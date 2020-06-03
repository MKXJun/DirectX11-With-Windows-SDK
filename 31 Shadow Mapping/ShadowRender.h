//***************************************************************************************
// ShadowRender.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// 阴影渲染类
// Shadow render class.
//***************************************************************************************

#ifndef SHADOWRENDER_H
#define SHADOWRENDER_H

#include <string>
#include <d3d11_1.h>
#include <wrl/client.h>

class ShadowRender
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ShadowRender() = default;
	~ShadowRender() = default;

	ShadowRender(const ShadowRender& rhs) = delete;
	ShadowRender& operator=(const ShadowRender& rhs) = delete;

	ShadowRender(ShadowRender&&) = default;
	ShadowRender& operator=(ShadowRender&&) = default;

	HRESULT InitResource(ID3D11Device* device, UINT width, UINT height);

	ID3D11ShaderResourceView* GetSRV();

	// 开始渲染到深度缓冲区
	void Begin(ID3D11DeviceContext* deviceContext);
	// 结束渲染
	void End(ID3D11DeviceContext* deviceContext);

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	
	D3D11_VIEWPORT m_ViewPort = {};

	UINT m_Width = 0;
	UINT m_Height = 0;

	DXGI_FORMAT m_Format = DXGI_FORMAT_R24G8_TYPELESS;

	ComPtr<ID3D11Texture2D> m_pDepthBuffer;
	ComPtr<ID3D11ShaderResourceView> m_pDepthBufferSRV;
	ComPtr<ID3D11DepthStencilView> m_pDepthBufferDSV;

	D3D11_VIEWPORT m_CachedViewPort = {};
	ComPtr<ID3D11RenderTargetView> m_pCachedRTV;
	ComPtr<ID3D11DepthStencilView> m_pCachedDSV;
};

#endif
