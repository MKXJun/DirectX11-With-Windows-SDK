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
	SkyRender(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		const std::wstring& cubemapFilename,
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	// 需要提供天空盒的六张正方形贴图
	SkyRender(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		const std::vector<std::wstring>& cubemapFilenames,
		float skySphereRadius,		// 天空球半径
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps


	ID3D11ShaderResourceView* GetTextureCube();

	void Draw(ID3D11DeviceContext* deviceContext, SkyEffect& skyEffect, const Camera& camera);

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	void InitResource(ID3D11Device* device, float skySphereRadius);

private:
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;

	UINT m_IndexCount;

	ComPtr<ID3D11ShaderResourceView> m_pTextureCubeSRV;
};

class DynamicSkyRender : public SkyRender
{
public:
	DynamicSkyRender(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		const std::wstring& cubemapFilename,
		float skySphereRadius,		// 天空球半径
		int dynamicCubeSize,		// 立方体棱长
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps
									// 动态天空盒必然生成mipmaps

	DynamicSkyRender(ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		const std::vector<std::wstring>& cubemapFilenames,
		float skySphereRadius,		// 天空球半径
		int dynamicCubeSize,		// 立方体棱长
		bool generateMips = false);	// 默认不为静态天空盒生成mipmaps
									// 动态天空盒必然生成mipmaps


	// 缓存当前渲染目标视图
	void Cache(ID3D11DeviceContext* deviceContext, BasicEffect& effect);

	// 指定天空盒某一面开始绘制，需要先调用Cache方法
	void BeginCapture(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const DirectX::XMFLOAT3& pos,
		D3D11_TEXTURECUBE_FACE face, float nearZ = 1e-3f, float farZ = 1e3f);

	// 恢复渲染目标视图及摄像机，并绑定当前动态天空盒
	void Restore(ID3D11DeviceContext* deviceContext, BasicEffect& effect, const Camera& camera);

	// 获取动态天空盒
	// 注意：该方法只能在Restore后再调用
	ID3D11ShaderResourceView* GetDynamicTextureCube();

	// 获取当前用于捕获的天空盒
	const Camera& GetCamera() const;

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:
	void InitResource(ID3D11Device* device, int dynamicCubeSize);

private:
	ComPtr<ID3D11RenderTargetView>		m_pCacheRTV;		        // 临时缓存的后备缓冲区
	ComPtr<ID3D11DepthStencilView>		m_pCacheDSV;		        // 临时缓存的深度/模板缓冲区

	FirstPersonCamera					m_pCamera;				    // 捕获当前天空盒其中一面的摄像机
	ComPtr<ID3D11DepthStencilView>		m_pDynamicCubeMapDSV;		// 动态天空盒渲染对应的深度/模板视图
	ComPtr<ID3D11ShaderResourceView>	m_pDynamicCubeMapSRV;		// 动态天空盒对应的着色器资源视图
	ComPtr<ID3D11RenderTargetView>		m_pDynamicCubeMapRTVs[6];	// 动态天空盒每个面对应的渲染目标视图

};

#endif
