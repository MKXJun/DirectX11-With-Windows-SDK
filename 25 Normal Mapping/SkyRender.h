//***************************************************************************************
// SkyRender.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 天空盒加载与渲染类
// Skybox loader and render classes.
//***************************************************************************************

#ifndef SKYRENDER_H
#define SKYRENDER_H

#include <vector>
#include <string>
#include "Effects.h"
#include "Camera.h"

class SkyRender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


	// 需要提供完整的天空盒贴图 或者 已经创建好的天空盒纹理.dds文件
	SkyRender(ComPtr<ID3D11Device> device, 
		ComPtr<ID3D11DeviceContext> deviceContext, 
		const std::wstring& cubemapFilename, 
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	// 需要提供天空盒的六张正方形贴图
	SkyRender(ComPtr<ID3D11Device> device, 
		ComPtr<ID3D11DeviceContext> deviceContext, 
		const std::vector<std::wstring>& cubemapFilenames, 
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	ComPtr<ID3D11ShaderResourceView> GetTextureCube();

	virtual void Draw(ComPtr<ID3D11DeviceContext> deviceContext, SkyEffect& skyEffect, const Camera& camera);

protected:
	void InitResource(ComPtr<ID3D11Device> device, float skySphereRadius);

protected:
	ComPtr<ID3D11Buffer> mVertexBuffer;
	ComPtr<ID3D11Buffer> mIndexBuffer;

	UINT mIndexCount;

	ComPtr<ID3D11ShaderResourceView> mTextureCubeSRV;
};

class DynamicSkyRender : public SkyRender
{
public:
	DynamicSkyRender(ComPtr<ID3D11Device> device,
		ComPtr<ID3D11DeviceContext> deviceContext,
		const std::wstring& cubemapFilename,
		float skySphereRadius,		// 天空球半径
		int dynamicCubeSize,		// 立方体棱长
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps
									// 动态天空盒必然生成mipmaps

	DynamicSkyRender(ComPtr<ID3D11Device> device,
		ComPtr<ID3D11DeviceContext> deviceContext,
		const std::vector<std::wstring>& cubemapFilenames,
		float skySphereRadius,		// 天空球半径
		int dynamicCubeSize,		// 立方体棱长
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps
									// 动态天空盒必然生成mipmaps


	// 缓存当前渲染目标视图
	void Cache(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect);

	// 指定天空盒某一面开始绘制，需要先调用Cache方法
	void BeginCapture(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect, const DirectX::XMFLOAT3& pos,
		D3D11_TEXTURECUBE_FACE face, float nearZ = 1e-3f, float farZ = 1e3f);

	// 恢复渲染目标视图及摄像机，并绑定当前动态天空盒
	void Restore(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect, const Camera& camera);

	// 获取动态天空盒
	// 注意：该方法只能在Restore后再调用
	ComPtr<ID3D11ShaderResourceView> GetDynamicTextureCube();

	// 获取当前用于捕获的天空盒
	const Camera& GetCamera() const;

private:
	void InitResource(ComPtr<ID3D11Device> device, int dynamicCubeSize);

private:
	ComPtr<ID3D11RenderTargetView>		mCacheRTV;		// 临时缓存的后备缓冲区
	ComPtr<ID3D11DepthStencilView>		mCacheDSV;		// 临时缓存的深度/模板缓冲区
	
	FirstPersonCamera					mCamera;				// 捕获当前天空盒其中一面的摄像机
	ComPtr<ID3D11DepthStencilView>		mDynamicCubeMapDSV;		// 动态天空盒渲染对应的深度/模板视图
	ComPtr<ID3D11ShaderResourceView>	mDynamicCubeMapSRV;		// 动态天空盒对应的着色器资源视图
	ComPtr<ID3D11RenderTargetView>		mDynamicCubeMapRTVs[6];	// 动态天空盒每个面对应的渲染目标视图
	
};

#endif
