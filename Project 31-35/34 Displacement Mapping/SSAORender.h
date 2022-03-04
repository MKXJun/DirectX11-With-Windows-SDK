//***************************************************************************************
// SSAORender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// SSAO渲染类
// SSAO Render class.
//***************************************************************************************
#ifndef SSAORENDER_H
#define SSAORENDER_H

#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>
#include "Effects.h"
#include "Camera.h"

class SSAORender
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	SSAORender() = default;
	~SSAORender() = default;
	// 不允许拷贝，允许移动
	SSAORender(const SSAORender&) = delete;
	SSAORender& operator=(const SSAORender&) = delete;
	SSAORender(SSAORender&&) = default;
	SSAORender& operator=(SSAORender&&) = default;

	HRESULT InitResource(ID3D11Device* device,
		int width, int height, float fovY, float farZ);

	HRESULT OnResize(ID3D11Device* device,
		int width, int height, float fovY, float farZ);

	// 开始绘制场景到法向量/深度图
	// 缓存当前RTV、DSV和视口
	// 当我们渲染到法向量/深度图时，需要传递程序使用的深度缓冲区
	void Begin(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* dsv, bool enableAlphaClip = false);
	// 完成最终的SSAO图绘制后进行恢复
	void End(ID3D11DeviceContext* deviceContext);

	// 设置SSAO图作为RTV，并绘制到一个全屏矩形以启用像素着色器来计算环境光遮蔽项
	// 尽管我们仍保持主深度缓冲区绑定到管线上，但要禁用深度缓冲区的读写，
	// 因为我们不需要深度缓冲区来计算SSAO图
	void ComputeSSAO(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, const Camera& camera);

	// 对SSAO图进行模糊，使得由于每个像素的采样次数较少而产生的噪点进行平滑处理
	// 这里使用边缘保留的模糊
	void BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, int blurCount);

	// 获取SSAO图
	ID3D11ShaderResourceView* GetAmbientTexture();

	// 设置调试对象名
	void SetDebugObjectName(const std::string& name);

private:

	void BuildFrustumFarCorners(float fovY, float farZ);
	void BuildOffsetVectors();
	HRESULT BuildTextureViews(ID3D11Device* device, int width, int height);
	HRESULT BuildFullScreenQuad(ID3D11Device* device);
	HRESULT BuildRandomVectorTexture(ID3D11Device* device);


	void BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect,
		ID3D11ShaderResourceView* inputSRV, ID3D11RenderTargetView* outputRTV, bool horzBlur);

private:
	UINT m_Width = 0;
	UINT m_Height = 0;

	DirectX::XMFLOAT4 m_FrustumFarCorner[4] = {};

	DirectX::XMFLOAT4 m_Offsets[14] = {};

	D3D11_VIEWPORT m_AmbientMapViewPort = {};

	ComPtr<ID3D11ShaderResourceView> m_pRandomVectorSRV;

	ComPtr<ID3D11RenderTargetView> m_pNormalDepthRTV;
	ComPtr<ID3D11ShaderResourceView> m_pNormalDepthSRV;

	ComPtr<ID3D11Buffer> m_pScreenQuadVB;
	ComPtr<ID3D11Buffer> m_pScreenQuadIB;

	ComPtr<ID3D11RenderTargetView> m_pCachedRTV;
	ComPtr<ID3D11DepthStencilView> m_pCachedDSV;
	D3D11_VIEWPORT m_CachedViewPort = {};

	//
	// 在模糊期间使用两个纹理进行Ping-Pong交换
	//

	ComPtr<ID3D11RenderTargetView> m_pAmbientRTV0;
	ComPtr<ID3D11ShaderResourceView> m_pAmbientSRV0;

	ComPtr<ID3D11RenderTargetView> m_pAmbientRTV1;
	ComPtr<ID3D11ShaderResourceView> m_pAmbientSRV1;
};

#endif
