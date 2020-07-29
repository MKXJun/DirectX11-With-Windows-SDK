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
	enum RSFillMode { Solid, WireFrame };

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

class IEffectTextureNormalMap
{
public:
	virtual void SetTextureNormalMap(ID3D11ShaderResourceView* textureNormal) = 0;
};

class IEffectDisplacementMap : public IEffectTextureNormalMap
{
public:
	virtual void SetEyePos(const DirectX::XMFLOAT3& eyePos) = 0;
	virtual void SetHeightScale(float scale) = 0;
	virtual void SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor) = 0;
};

class BasicEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse, public IEffectDisplacementMap
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
	// IEffectTransform
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	//
	// IEffectTextureDiffuse
	//

	// 设置漫反射纹理
	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	//
	// IEffectDisplacementMap
	//

	// 设置法线/位移贴图
	void SetTextureNormalMap(ID3D11ShaderResourceView* textureNormalMap) override;
	// 设置摄像机位置
	void SetEyePos(const DirectX::XMFLOAT3& eyePos) override;
	// 设置位移幅度
	void SetHeightScale(float scale) override;
	// 设置曲面细分信息
	void SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor) override;

	// 
	// BasicEffect
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext * deviceContext, RenderType type, RSFillMode fillMode = RSFillMode::Solid);
	// 带法线贴图的绘制
	void SetRenderWithNormalMap(ID3D11DeviceContext* deviceContext, RenderType type, RSFillMode fillMode = RSFillMode::Solid);
	// 带位移映射的绘制
	void SetRenderWithDisplacementMap(ID3D11DeviceContext* deviceContext, RenderType type, RSFillMode fillMode = RSFillMode::Solid);


	void XM_CALLCONV SetShadowTransformMatrix(DirectX::FXMMATRIX S);

	// 各种类型灯光允许的最大数目
	static const int maxLights = 5;

	void SetDirLight(size_t pos, const DirectionalLight& dirLight);
	void SetPointLight(size_t pos, const PointLight& pointLight);
	void SetSpotLight(size_t pos, const SpotLight& spotLight);
	void SetMaterial(const Material& material);

	// 是否使用纹理
	void SetTextureUsed(bool isUsed);
	// 是否使用阴影
	void SetShadowEnabled(bool enabled);
	// 是否使用SSAO
	void SetSSAOEnabled(bool enabled);

	// 设置阴影贴图
	void SetTextureShadowMap(ID3D11ShaderResourceView* textureShadowMap);
	// 设置SSAO图
	void SetTextureSSAOMap(ID3D11ShaderResourceView* textureSSAOMap);
	// 设置天空盒
	void SetTextureCube(ID3D11ShaderResourceView* textureCube);

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;
	
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
	// IEffectTransform
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	// 
	// SkyEffect
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 设置天空盒
	void SetTextureCube(ID3D11ShaderResourceView* textureCube);

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class ShadowEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse, public IEffectDisplacementMap
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
	// IEffectTransform
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	// 
	// IEffectTextureDiffuse
	//

	// 设置漫反射纹理
	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	// 
	// IEffectDisplacementMap
	//

	// 设置法线/位移贴图
	void SetTextureNormalMap(ID3D11ShaderResourceView* textureNormalMap) override;
	// 设置摄像机位置
	void SetEyePos(const DirectX::XMFLOAT3& eyePos) override;
	// 设置位移幅度
	void SetHeightScale(float scale) override;
	// 设置曲面细分信息
	void SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor) override;

	// 
	// ShadowEffect
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext, RenderType type);

	// Alpha裁剪绘制(处理具有透明度的物体)
	void SetRenderAlphaClip(ID3D11DeviceContext* deviceContext, RenderType type);

	// 带位移贴图的绘制
	void SetRenderWithDisplacementMap(ID3D11DeviceContext* deviceContext, RenderType type);

	// 带位移映射的Alpha裁剪绘制(处理具有透明度的物体)
	void SetRenderAlphaClipWithDisplacementMap(ID3D11DeviceContext* deviceContext, RenderType type);

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;

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
	// IEffectTransform
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	//
	// IEffectTextureDiffuse
	//

	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	// 
	// DebugEffect
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 绘制单通道(0-R, 1-G, 2-B)
	void SetRenderOneComponent(ID3D11DeviceContext* deviceContext, int index);

	// 绘制单通道，但以灰度的形式呈现(0-R, 1-G, 2-B, 3-A)
	void SetRenderOneComponentGray(ID3D11DeviceContext* deviceContext, int index);

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class SSAOEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse, public IEffectDisplacementMap
{
public:

	SSAOEffect();
	virtual ~SSAOEffect() override;

	SSAOEffect(SSAOEffect&& moveFrom) noexcept;
	SSAOEffect& operator=(SSAOEffect&& moveFrom) noexcept;

	// 获取单例
	static SSAOEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device);

	//
	// IEffectTextureDiffuse
	//

	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	//
	// IEffectTransform
	//

	// 设置漫反射纹理
	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;

	//
	// IEffectDisplacementMap
	//

	// 设置法线/位移贴图
	void SetTextureNormalMap(ID3D11ShaderResourceView* textureNormalMap) override;
	// 设置摄像机位置
	void SetEyePos(const DirectX::XMFLOAT3& eyePos) override;
	// 设置位移幅度
	void SetHeightScale(float scale) override;
	// 设置曲面细分信息
	void SetTessInfo(float maxTessDistance, float minTessDistance, float minTessFactor, float maxTessFactor) override;

	// 
	// SSAOEffect
	//

	// 绘制法向量和深度贴图
	void SetRenderNormalDepth(ID3D11DeviceContext* deviceContext, RenderType type, bool enableAlphaClip = false);

	// 绘制带位移映射的法向量和深度贴图
	void SetRenderNormalDepthWithDisplacementMap(ID3D11DeviceContext* deviceContext, RenderType type, bool enableAlphaClip = false);

	// 绘制SSAO图
	void SetRenderSSAOMap(ID3D11DeviceContext* deviceContext, int sampleCount);

	// 对SSAO图进行双边滤波
	void SetRenderBilateralBlur(ID3D11DeviceContext* deviceContext, bool horizontalBlur);

	// 设置观察空间的深度/法向量贴图
	void SetTextureNormalDepth(ID3D11ShaderResourceView* textureNormalDepth);
	// 设置随机向量纹理
	void SetTextureRandomVec(ID3D11ShaderResourceView* textureRandomVec);
	// 设置待模糊的纹理
	void SetTextureBlur(ID3D11ShaderResourceView* textureBlur);
	// 设置偏移向量
	void SetOffsetVectors(const DirectX::XMFLOAT4 offsetVectors[14]);
	// 设置视锥体远平面顶点
	void SetFrustumCorners(const DirectX::XMFLOAT4 frustumCorners[4]);
	// 设置遮蔽信息
	void SetOcclusionInfo(float radius, float fadeStart, float fadeEnd, float surfaceEpsilon);
	// 设置模糊权值
	void SetBlurWeights(const float weights[11]);
	// 设置模糊半径
	void SetBlurRadius(int radius);

	// 
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};


#endif
