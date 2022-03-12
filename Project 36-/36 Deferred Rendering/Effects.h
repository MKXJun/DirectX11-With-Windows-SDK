//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易特效管理框架
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include "IEffect.h"
#include "RenderStates.h"

class ForwardEffect : public IEffect, public IEffectTransform, 
	public IEffectTextureDiffuse
{
public:
	ForwardEffect();
	virtual ~ForwardEffect() override;

	ForwardEffect(ForwardEffect&& moveFrom) noexcept;
	ForwardEffect& operator=(ForwardEffect&& moveFrom) noexcept;

	// 获取单例
	static ForwardEffect& Get();

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
	// ForwardEffect
	//

	//
	void SetLightBuffer(ID3D11ShaderResourceView* lightBuffer);

	// 进行Pre-Z通道绘制
	void SetRenderPreZPass(ID3D11DeviceContext* deviceContext);
	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class SkyboxToneMapEffect : public IEffect, public IEffectTransform
{
public:
	SkyboxToneMapEffect();
	virtual ~SkyboxToneMapEffect() override;

	SkyboxToneMapEffect(SkyboxToneMapEffect&& moveFrom) noexcept;
	SkyboxToneMapEffect& operator=(SkyboxToneMapEffect&& moveFrom) noexcept;

	// 获取单例
	static SkyboxToneMapEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device);


	//
	// IEffectTransform
	//

	// 无用
	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
	
	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;

	// 
	// SkyboxToneMapEffect
	//

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 设置天空盒
	void SetSkyboxTexture(ID3D11ShaderResourceView* skyboxTexture);
	// 设置深度图
	void SetDepthTexture(ID3D11ShaderResourceView* depthTexture);
	// 设置场景渲染图
	void SetLitTexture(ID3D11ShaderResourceView* litTexture);

	// 设置MSAA采样等级
	void SetMsaaSamples(UINT msaaSamples);

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext * deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class DeferredEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse
{
public:
	// 光源裁剪技术
	enum LightCullTechnique {
		CULL_FORWARD_NONE = 0,    // 前向渲染，无光照裁剪
		CULL_FORWARD_PREZ_NONE,   // 前向渲染，预写入深度，无光照裁剪
		CULL_DEFERRED_NONE,       // 传统延迟渲染
	};

	DeferredEffect();
	virtual ~DeferredEffect() override;

	DeferredEffect(DeferredEffect&& moveFrom) noexcept;
	DeferredEffect& operator=(DeferredEffect&& moveFrom) noexcept;

	// 获取单例
	static DeferredEffect& Get();

	// 初始化所需资源
	bool InitAll(ID3D11Device* device, UINT msaaSamples = 1);

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
	// BasicDeferredEffect
	//

	void SetMsaaSamples(UINT msaaSamples);

	void SetLightingOnly(bool enable);
	void SetFaceNormals(bool enable);
	void SetVisualizeLightCount(bool enable);
	void SetVisualizeShadingFreq(bool enable);

	void SetCameraNearFar(float nearZ, float farZ);

	// 绘制G缓冲区
	void SetRenderGBuffer(ID3D11DeviceContext* deviceContext);
	
	// 将法线G-Buffer渲染到目标纹理
	void DebugNormalGBuffer(ID3D11DeviceContext* deviceContext,
		ID3D11RenderTargetView* rtv,
		ID3D11ShaderResourceView* normalGBuffer,
		D3D11_VIEWPORT viewport);

	// 传统延迟渲染
	void ComputeLightingDefault(ID3D11DeviceContext* deviceContext,
		ID3D11RenderTargetView* litBufferRTV,
		ID3D11DepthStencilView* depthBufferReadOnlyDSV,
		ID3D11ShaderResourceView* lightBufferSRV,
		ID3D11ShaderResourceView* GBuffers[4],
		D3D11_VIEWPORT viewport);

	

	//
	// IEffect
	//

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

class DebugEffect : public IEffect, public IEffectTextureDiffuse
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
	void Apply(ID3D11DeviceContext* deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

#endif
