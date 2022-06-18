//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易特效管理框架
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include <IEffect.h>
#include <Material.h>
#include <MeshData.h>
#include <LightHelper.h>

class BasicEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
{
public:

    BasicEffect();
    virtual ~BasicEffect() override;

    BasicEffect(BasicEffect&& moveFrom) noexcept;
    BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;

    // 获取单例
    static BasicEffect& Get();

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

    void SetMaterial(const Material& material) override;

    //
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;

    //
    // BasicEffect
    //

    // 默认状态来绘制
    void SetRenderDefault();
    // 透明混合绘制
    void SetRenderTransparent();

    // OIT Pass1

    // 清空OIT缓冲区
    void ClearOITBuffers(
        ID3D11DeviceContext* deviceContext,
        ID3D11UnorderedAccessView* flBuffer,
        ID3D11UnorderedAccessView* startOffsetBuffer);

    // 顺序无关透明度存储
    void SetRenderOITStorage(
        ID3D11UnorderedAccessView* flBuffer,
        ID3D11UnorderedAccessView* startOffsetBuffer,
        uint32_t renderTargetWidth);

    // OIT Pass2

    // 完成OIT渲染
    void RenderOIT(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* flBuffer,
        ID3D11ShaderResourceView* startOffsetBuffer,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

    void SetTextureDisplacement(ID3D11ShaderResourceView* textureDisplacement);

    // 各种类型灯光允许的最大数目
    static const int maxLights = 5;

    void SetDirLight(uint32_t pos, const DirectionalLight& dirLight);
    void SetPointLight(uint32_t pos, const PointLight& pointLight);
    void SetSpotLight(uint32_t pos, const SpotLight& spotLight);

    void SetEyePos(const DirectX::XMFLOAT3& eyePos);

    void SetFogState(bool isOn);
    void SetFogStart(float fogStart);
    void SetFogColor(const DirectX::XMFLOAT4& fogColor);
    void SetFogRange(float fogRange);


    void SetWavesStates(bool enabled, float gridSpatialStep = 0.0f);

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class PostProcessEffect
{
public:

    PostProcessEffect();
    ~PostProcessEffect();

    PostProcessEffect(PostProcessEffect&& moveFrom) noexcept;
    PostProcessEffect& operator=(PostProcessEffect&& moveFrom) noexcept;

    // 获取单例
    static PostProcessEffect& Get();

    // 初始化所需资源
    bool InitAll(ID3D11Device* device);

    //
    // 将两张图片进行分量相乘
    // input2为nullptr则直通
    //
    void RenderComposite(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input1,
        ID3D11ShaderResourceView* input2,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp
    );

    //
    // Sobel滤波
    //
    void ComputeSobel(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11UnorderedAccessView* output,
        uint32_t width, uint32_t height);

    //
    // 高斯滤波
    //

    // size: 奇数, 3-19
    void SetBlurKernelSize(int size);
    void SetBlurSigma(float sigma);

    void ComputeGaussianBlurX(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11UnorderedAccessView* output,
        uint32_t width, uint32_t height);

    void ComputeGaussianBlurY(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11UnorderedAccessView* output,
        uint32_t width, uint32_t height);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif
