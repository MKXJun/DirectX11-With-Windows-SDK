//***************************************************************************************
// IEffects.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// 特效基类定义
// Effect interface definitions.
//***************************************************************************************

#ifndef IEFFECT_H
#define IEFFECT_H

#include <memory>
#include <d3d11_1.h>
#include <DirectXMath.h>


class IEffect
{
public:
	enum RenderType { RenderObject, RenderInstance };
	enum RSFillMode { Solid, WireFrame };

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

class IEffectMaterial
{
public:
	virtual void XM_CALLCONV SetAmbinet(const DirectX::XMFLOAT4& ambient) = 0;
	virtual void XM_CALLCONV SetDiffuse(const DirectX::XMFLOAT4& diffuse) = 0;
	virtual void XM_CALLCONV SetSpecular(const DirectX::XMFLOAT4& specular) = 0;
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

#endif
