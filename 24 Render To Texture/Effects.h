//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易特效管理框架
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include <memory>
#include "LightHelper.h"
#include "RenderStates.h"


class IEffect
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	IEffect() = default;

	// 不支持复制构造
	IEffect(const IEffect&) = delete;
	IEffect& operator=(const IEffect&) = delete;

	// 允许转移
	IEffect(IEffect&& moveFrom) = default;
	IEffect& operator=(IEffect&& moveFrom) = default;

	virtual ~IEffect() = default;

	// 更新并绑定常量缓冲区
	virtual void Apply(ComPtr<ID3D11DeviceContext> deviceContext) = 0;
};


class BasicEffect : public IEffect
{
public:

	enum RenderType { RenderObject, RenderInstance };

	BasicEffect();
	virtual ~BasicEffect() override;

	BasicEffect(BasicEffect&& moveFrom) noexcept;
	BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;

	// 获取单例
	static BasicEffect& Get();

	

	// 初始化Basic.hlsli所需资源并初始化渲染状态
	bool InitAll(ComPtr<ID3D11Device> device);


	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext, RenderType type);

	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W);
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);

	//
	// 光照、材质和纹理相关设置
	//

	// 各种类型灯光允许的最大数目
	static const int maxLights = 5;

	void SetDirLight(size_t pos, const DirectionalLight& dirLight);
	void SetPointLight(size_t pos, const PointLight& pointLight);
	void SetSpotLight(size_t pos, const SpotLight& spotLight);

	void SetMaterial(const Material& material);


	void SetTextureUsed(bool isUsed);

	void SetTextureDiffuse(ComPtr<ID3D11ShaderResourceView> texture);

	void XM_CALLCONV SetEyePos(DirectX::FXMVECTOR eyePos);
	
	//
	// 雾效设置
	//

	void SetFogState(bool isOn);
	void SetFogStart(float fogStart);
	void SetFogColor(DirectX::XMVECTOR fogColor);
	void SetFogRange(float fogRange);

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ComPtr<ID3D11DeviceContext> deviceContext);
	
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class ScreenFadeEffect : public IEffect
{
public:
	ScreenFadeEffect();
	virtual ~ScreenFadeEffect() override;

	ScreenFadeEffect(ScreenFadeEffect&& moveFrom);
	ScreenFadeEffect& operator=(ScreenFadeEffect&& moveFrom);

	// 获取单例
	static ScreenFadeEffect& Get();

	// 初始化ScreenFade.hlsli所需资源并初始化渲染状态
	bool InitAll(ComPtr<ID3D11Device> device);

	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext);

	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldViewProjMatrix(DirectX::FXMMATRIX W, DirectX::CXMMATRIX V, DirectX::CXMMATRIX P);
	void XM_CALLCONV SetWorldViewProjMatrix(DirectX::FXMMATRIX WVP);

	//
	// 淡入淡出设置
	//

	void SetFadeAmount(float fadeAmount);

	//
	// 纹理设置
	//

	void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ComPtr<ID3D11DeviceContext> deviceContext);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class MinimapEffect : public IEffect
{
public:
	MinimapEffect();
	virtual ~MinimapEffect() override;

	MinimapEffect(MinimapEffect&& moveFrom);
	MinimapEffect& operator=(MinimapEffect&& moveFrom);

	// 获取单例
	static MinimapEffect& Get();

	// 初始化Minimap.hlsli所需资源并初始化渲染状态
	bool InitAll(ComPtr<ID3D11Device> device);

	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ComPtr<ID3D11DeviceContext> deviceContext);

	//
	// 状态设置
	//

	void SetFogState(bool isOn);
	void SetVisibleRange(float range);
	void XM_CALLCONV SetEyePos(DirectX::FXMVECTOR eyePos);
	void XM_CALLCONV SetMinimapRect(DirectX::FXMVECTOR rect); // (Left, Front, Right, Back)
	void XM_CALLCONV SetInvisibleColor(DirectX::FXMVECTOR color);
	
	//
	// 纹理设置
	//

	void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);


	// 应用常量缓冲区和纹理资源的变更
	void Apply(ComPtr<ID3D11DeviceContext> deviceContext);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};




#endif
