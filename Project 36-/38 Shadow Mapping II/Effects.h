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

class ForwardEffect : public IEffect, public IEffectTransform, 
	public IEffectMaterial, public IEffectMeshData
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
	// IEffectMaterial
	//

	void SetMaterial(Material& material) override;

	//
	// IEffectMeshData
	//

	MeshDataInput GetInputData(MeshData& meshData) override;


	//
	// ForwardEffect
	//
	void SetLightPos(const DirectX::XMFLOAT3& pos);
	void SetLightIntensity(float intensity);
	void SetSpecular(const DirectX::XMFLOAT4& specular);

	void XM_CALLCONV SetShadowTransformMatrix(DirectX::FXMMATRIX S);

	// 默认状态来绘制
	void SetRenderDefault(ID3D11DeviceContext* deviceContext);

	// 应用常量缓冲区和纹理资源的变更
	void Apply(ID3D11DeviceContext* deviceContext) override;

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

//class ShadowEffect : public IEffect, public IEffectTransform, public IEffectTextureDiffuse
//{
//public:
//	ShadowEffect();
//	virtual ~ShadowEffect() override;
//
//	ShadowEffect(ShadowEffect&& moveFrom) noexcept;
//	ShadowEffect& operator=(ShadowEffect&& moveFrom) noexcept;
//
//	// 获取单例
//	static ShadowEffect& Get();
//
//	// 初始化所需资源
//	bool InitAll(ID3D11Device* device);
//
//	//
//	// IEffectTransform
//	//
//
//	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) override;
//	void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) override;
//	void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) override;
//
//	//
//	// IEffectTextureDiffuse
//	//
//
//	// 设置漫反射纹理
//	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;
//
//	// 
//	// ShadowEffect
//	//
//
//	// 默认状态来绘制
//	void SetRenderDefault(ID3D11DeviceContext* deviceContext);
//
//	// Alpha裁剪绘制(处理具有透明度的物体)
//	void SetRenderAlphaClip(ID3D11DeviceContext* deviceContext);
//
//	//
//	// IEffect
//	//
//
//	// 应用常量缓冲区和纹理资源的变更
//	void Apply(ID3D11DeviceContext* deviceContext) override;
//
//private:
//	class Impl;
//	std::unique_ptr<Impl> pImpl;
//};

class SkyboxToneMapEffect : public IEffect, public IEffectTransform,
	public IEffectMaterial, public IEffectMeshData
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
	// IEffectMaterial
	//

	void SetMaterial(Material& material) override;

	//
	// IEffectMeshData
	//

	MeshDataInput GetInputData(MeshData& meshData) override;

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

//class DebugEffect : public IEffect, public IEffectTextureDiffuse
//{
//public:
//	DebugEffect();
//	virtual ~DebugEffect() override;
//
//	DebugEffect(DebugEffect&& moveFrom) noexcept;
//	DebugEffect& operator=(DebugEffect&& moveFrom) noexcept;
//
//	// 获取单例
//	static DebugEffect& Get();
//
//	// 初始化所需资源
//	bool InitAll(ID3D11Device* device);
//
//	//
//	// IEffectTextureDiffuse
//	//
//
//	void SetTextureDiffuse(ID3D11ShaderResourceView* textureDiffuse) override;
//
//	// 
//	// DebugEffect
//	//
//
//	// 默认状态来绘制
//	void SetRenderDefault(ID3D11DeviceContext* deviceContext);
//
//	// 绘制单通道(0-R, 1-G, 2-B)
//	void SetRenderOneComponent(ID3D11DeviceContext* deviceContext, int index);
//
//	// 绘制单通道，但以灰度的形式呈现(0-R, 1-G, 2-B, 3-A)
//	void SetRenderOneComponentGray(ID3D11DeviceContext* deviceContext, int index);
//
//	//
//	// IEffect
//	//
//
//	// 应用常量缓冲区和纹理资源的变更
//	void Apply(ID3D11DeviceContext* deviceContext) override;
//
//private:
//	class Impl;
//	std::unique_ptr<Impl> pImpl;
//};

#endif
