//***************************************************************************************
// TextureRender.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 渲染到纹理类
// Render-To-Texture class.
//***************************************************************************************

#ifndef TEXTURERENDER_H
#define TEXTURERENDER_H

#include <d3d11_1.h>
#include <wrl/client.h>

class TextureRender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


	TextureRender(ComPtr<ID3D11Device> device,
		int texWidth, 
		int texHeight,
		bool generateMips = false);
	~TextureRender();

	// 开始对当前纹理进行渲染
	void Begin(ComPtr<ID3D11DeviceContext> deviceContext);
	// 结束对当前纹理的渲染，还原状态
	void End(ComPtr<ID3D11DeviceContext> deviceContext);
	// 获取渲染好的纹理
	ComPtr<ID3D11ShaderResourceView> GetOutputTexture();

private:
	ComPtr<ID3D11ShaderResourceView>	mOutputTextureSRV;	// 输出的纹理对应的着色器资源视图
	ComPtr<ID3D11RenderTargetView>		mOutputTextureRTV;	// 输出的纹理对应的渲染目标视图
	ComPtr<ID3D11DepthStencilView>		mOutputTextureDSV;	// 输出纹理所用的深度/模板视图
	D3D11_VIEWPORT						mOutputViewPort;	// 输出所用的视口

	ComPtr<ID3D11RenderTargetView>		mCacheRTV;		// 临时缓存的后备缓冲区
	ComPtr<ID3D11DepthStencilView>		mCacheDSV;		// 临时缓存的深度/模板缓冲区
	D3D11_VIEWPORT						mCacheViewPort;	// 临时缓存的视口

	bool								mGenerateMips;	// 是否生成mipmap链
};

#endif