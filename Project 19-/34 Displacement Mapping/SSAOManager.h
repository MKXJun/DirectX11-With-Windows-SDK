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
#include <string_view>
#include "Effects.h"
#include <Texture2D.h>
#include <Buffer.h>
#include <Camera.h>

class SSAOManager
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    SSAOManager() = default;
    ~SSAOManager() = default;
    // 不允许拷贝，允许移动
    SSAOManager(const SSAOManager&) = delete;
    SSAOManager& operator=(const SSAOManager&) = delete;
    SSAOManager(SSAOManager&&) = default;
    SSAOManager& operator=(SSAOManager&&) = default;

    void InitResource(ID3D11Device* device, int width, int height);

    void OnResize(ID3D11Device* device, int width, int height);

    void SetSampleCount(uint32_t count) { m_SampleCount = count; }



    // 绑定深度缓冲区、法线/深度图到管线并初始化
    void Begin(ID3D11DeviceContext* deviceContext, ID3D11DepthStencilView* dsv, const D3D11_VIEWPORT& vp);
    // 完成法向量/深度图绘制后清除绑定
    void End(ID3D11DeviceContext* deviceContext);

    // 设置SSAO图作为RTV，并绘制到一个全屏矩形以启用像素着色器来计算环境光遮蔽项
    // 尽管我们仍保持主深度缓冲区绑定到管线上，但要禁用深度缓冲区的读写，
    // 因为我们不需要深度缓冲区来计算SSAO图
    void RenderToSSAOTexture(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect, const Camera& camera);

    // 对SSAO图进行模糊，使得由于每个像素的采样次数较少而产生的噪点进行平滑处理
    // 这里使用边缘保留的模糊
    void BlurAmbientMap(ID3D11DeviceContext* deviceContext, SSAOEffect& ssaoEffect);



    // 获取环境光遮蔽图
    ID3D11ShaderResourceView* GetAmbientOcclusionTexture();
    // 获取法线深度贴图
    ID3D11ShaderResourceView* GetNormalDepthTexture();

public:
    // SSAO默认设置，可进行修改
    uint32_t m_SampleCount = 14;        // 采样向量数
    uint32_t m_BlurCount = 4;           // 模糊次数
    float m_OcclusionRadius = 0.5f;     // 采样半球的半径
    float m_OcclusionFadeStart = 0.2f;  // 开始向不遮蔽过渡的起始距离值
    float m_OcclusionFadeEnd = 2.0f;    // 完全不遮蔽的距离值
    float m_SurfaceEpsilon = 0.05f;     // 防止自相交用的距离值
    float m_BlurWeights[11]{ 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };
    uint32_t m_BlurRadius = 5;

private:

    void BuildOffsetVectors();
    void BuildRandomVectorTexture(ID3D11Device* device);

private:

    DirectX::XMFLOAT4 m_Offsets[14] = {};

    std::unique_ptr<Texture2D> m_pNormalDepthTexture;           // 法线/深度纹理
    std::unique_ptr<Texture2D> m_pAOTexture;                    // 环境光遮蔽贴图
    std::unique_ptr<Texture2D> m_pAOTempTexture;                // 中间环境光遮蔽贴图
    std::unique_ptr<Texture2D> m_pRandomVectorTexture;          // 随机向量纹理
};

#endif
