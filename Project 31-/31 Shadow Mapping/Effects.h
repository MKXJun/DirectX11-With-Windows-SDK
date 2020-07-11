//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
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
	enum RenderType { RenderObject, RenderInstance };

	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	IEffect() = default;
	virtual ~IEffect() = default;
	// 不允许拷贝，允许移动
	IEffect(const IEffect&) = delete;
	IEffect& operator=(const IEffect&) = delete;
	IEffect(IEffect&&) = default;
	IEffect& operator=(IEffect&&) = default;

	// 更新并绑定常量缓冲区
	virtual void Apply(ID3D11DeviceContext * deviceContext) = 0;
};

class IEffectTransform
{
public:
	virtual void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) = 0;
	virtual void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) = 0;
	virtual void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) = 0;
};

class IEffectTextureDiffuse
{
public:
	virtual void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) = 0;
};

class BasicEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse
{
public:
	BasicEffect();
	virtual ~BasicEffect() override;

	BasicEffect(BasicEffect&& moveFrom) noexcept;
	BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;

	// 获取单例
	static BasicEffect& Get();

	

	// 初始化所需资源
	bool InitAll(ID3D11Device * device);


	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext * deviceContext, RenderType type);
	// 带法线贴图的绘制
	void SetRenderWithNormalMap(ID3D11DeviceContext* deviceContext, RenderType type);

	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;
	
	void XM_CALLCONV SetShadowTransformMatrix(DirectX::FXMMATRIX S);

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

	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	void SetTextureNormalMap(ID3D11ShaderResourceView * textureNormalMap);
	void SetTextureShadowMap(ID3D11ShaderResourceView * textureShadowMap);
	void SetTextureCube(ID3D11ShaderResourceView * textureCube);

	void XM_CALLCONV SetEyePos(DirectX::FXMVECTOR eyePos);
	

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext);
	
private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class SkyEffect : public IEffect, public IEffectTransform
{
public:
	SkyEffect();
	virtual ~SkyEffect() override;

	SkyEffect(SkyEffect&& moveFrom) noexcept;
	SkyEffect& operator=(SkyEffect&& moveFrom) noexcept;

	// 获取单例
	static SkyEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device);

	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	//
	// 纹理立方体映射设置
	//

	void SetTextureCube(ID3D11ShaderResourceView* textureCube);


	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class ShadowEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse
{
public:
	ShadowEffect();
	virtual ~ShadowEffect() override;

	ShadowEffect(ShadowEffect&& moveFrom) noexcept;
	ShadowEffect& operator=(ShadowEffect&& moveFrom) noexcept;

	// 获取单例
	static ShadowEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device);

	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext, RenderType type);

	// Alpha裁剪绘制(处理具有透明度的物体)
	void SetRenderAlphaClip(ID3D11DeviceContext* deviceContext, RenderType type);

	// 设置漫反射纹理
	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class DebugEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse
{
public:
	DebugEffect();
	virtual ~DebugEffect() override;

	DebugEffect(DebugEffect&& moveFrom) noexcept;
	DebugEffect& operator=(DebugEffect&& moveFrom) noexcept;

	// 获取单例
	static DebugEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device);

	// 
	// 渲染模式的变更
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 绘制单通道(0-R, 1-G, 2-B)
	void SetRenderOneComponent(ID3D11DeviceContext* deviceContext, int index);

	// 绘制单通道，但以灰度的形式呈现(0-R, 1-G, 2-B, 3-A)
	void SetRenderOneComponentGray(ID3D11DeviceContext* deviceContext, int index);


	//
	// 矩阵设置
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;


	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

#endif
