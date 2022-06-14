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
    void SetRenderDefault(ID3D11DeviceContext* deviceContext);
    // 透明混合绘制
    void SetRenderTransparent(ID3D11DeviceContext* deviceContext);
    // 顺序无关透明度存储
    void SetRenderOITStorage(
        ID3D11DeviceContext* deviceContext,
        ID3D11UnorderedAccessView* flBuffer,
        ID3D11UnorderedAccessView* startOffsetBuffer,
        uint32_t renderTargetWidth);

    // 完成OIT渲染
    void RenderOIT(ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* FLBuffer,
        ID3D11ShaderResourceView* startOffsetBuffer,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

    void SetTextureDisplacement(ID3D11ShaderResourceView* textureDisplacement, ID3D11DeviceContext* deviceContext = nullptr);

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

//class BasicEffect : public IEffect
//{
//public:
//
//    enum RenderType { RenderObject, RenderInstance };
//
//    BasicEffect();
//    virtual ~BasicEffect() override;
//
//    BasicEffect(BasicEffect&& moveFrom) noexcept;
//    BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;
//
//    // 获取单例
//    static BasicEffect& Get();
//
//    
//
//    // 初始化所需资源
//    bool InitAll(ID3D11Device * device);
//
//
//    // 
//    // 渲染模式的变更
//    //
//
//    // 默认状态来绘制
//    void SetRenderDefault(ID3D11DeviceContext * deviceContext, RenderType type);
//    // 2D默认状态绘制
//    void Set2DRenderDefault(ID3D11DeviceContext* deviceContext);
//    // 纹理合成渲染
//    void SetRenderComposite(ID3D11DeviceContext* deviceContext);
//
//    //
//    // 矩阵设置
//    //
//
//    void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W);
//    void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
//    void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);
//    void XM_CALLCONV SetTexTransformMatrix(DirectX::FXMMATRIX T);
//    
//    //
//    // 光照、材质和纹理相关设置
//    //
//
//    // 各种类型灯光允许的最大数目
//    static const int maxLights = 5;
//
//    void SetDirLight(size_t pos, const DirectionalLight& dirLight);
//    void SetPointLight(size_t pos, const PointLight& pointLight);
//    void SetSpotLight(size_t pos, const SpotLight& spotLight);
//
//    void SetMaterial(const Material& material);
//
//    void SetTextureDiffuse(ID3D11ShaderResourceView * textureDiffuse);
//    void SetTextureDisplacement(ID3D11ShaderResourceView * textureDisplacement);
//    void SetTextureComposite(ID3D11ShaderResourceView* textureComposite);
//
//    void SetWavesStates(bool enabled, float texelSizeU = 0.0f, float texelSizeV = 0.0f, float gridSpatialStep = 0.0f);
//
//
//    void SetEyePos(const DirectX::XMFLOAT3& eyePos);
//    
//
//    //
//    // 状态设置
//    //
//
//    void SetFogState(bool isOn);
//    void SetFogStart(float fogStart);
//    void SetFogColor(DirectX::XMVECTOR fogColor);
//    void SetFogRange(float fogRange);
//
//    // 应用常量缓冲区和纹理资源的变更
//    void Apply(ID3D11DeviceContext * deviceContext) override;
//    
//private:
//    class Impl;
//    std::unique_ptr<Impl> pImpl;
//};

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
