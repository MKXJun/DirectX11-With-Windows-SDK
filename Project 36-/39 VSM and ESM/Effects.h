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
#include "Material.h"
#include "MeshData.h"

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

    void SetMaterial(const Material& material) override;

    //
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;


    //
    // ForwardEffect
    //
    
    // 0: Cascaded Shadow Map
    // 1: Variance Shadow Map
    // 2: Exponmential Shadow Map
    void SetShadowType(int type);

    // 通用

    void SetCascadeLevels(int cascadeLevels);
    void SetCascadeBlendEnabled(bool enable);
    void SetCascadeIntervalSelectionEnabled(bool enable);
    void SetCascadeVisulization(bool enable);

    void SetCascadeOffsets(const DirectX::XMFLOAT4 offsets[8]);
    void SetCascadeScales(const DirectX::XMFLOAT4 scales[8]);
    void SetCascadeFrustumsEyeSpaceDepths(const float depths[8]);
    void SetCascadeBlendArea(float blendArea);
    void SetShadowSize(int size);
    void XM_CALLCONV SetShadowViewMatrix(DirectX::FXMMATRIX ShadowView);
    void SetShadowTextureArray(ID3D11ShaderResourceView* shadow);

    void SetLightBleedingReduction(float value);
    void SetCascadeSampler(ID3D11SamplerState* sampler);

    // CSM
    void SetPCFKernelSize(int size);
    void SetPCFDepthBias(float bias);

    // VSM
    void SetMagicPower(float power);




    void SetLightDir(const DirectX::XMFLOAT3& dir);


    // 默认状态来绘制
    void SetRenderDefault(ID3D11DeviceContext* deviceContext, bool reversedZ = false);
    // 进行Pre-Z通道绘制
    void SetRenderPreZPass(ID3D11DeviceContext* deviceContext, bool reversedZ = false);


    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class ShadowEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
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
    // IEffectMaterial
    //

    void SetMaterial(const Material& material) override;

    //
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;

    //
    // ShadowEffect
    //

    // 仅写入深度图
    void SetRenderDepthOnly(ID3D11DeviceContext* deviceContext);

    // 同时将深度绘制到深度图
    void SetRenderDefault(ID3D11DeviceContext* deviceContext);

    // 生成方差阴影
    void RenderVarianceShadow(ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

    // 生成指数阴影
    void RenderExponentialShadow(ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp,
        float magic_power);

    // 绘制深度图到纹理
    void RenderDepthToTexture(ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* input, 
        ID3D11RenderTargetView* output, 
        const D3D11_VIEWPORT& vp);

    // input和output纹理宽高要求一致
    // kernel_size: 奇数, 3-15
    void VarianceShadowHorizontialBlur(
        ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* input, 
        ID3D11RenderTargetView* output, 
        const D3D11_VIEWPORT& vp, 
        int kernel_size);

    // input和output纹理宽高要求一致
    // kernel_size: 奇数, 3-15
    void VarianceShadowVerticalBlur(
        ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* input, 
        ID3D11RenderTargetView* output, 
        const D3D11_VIEWPORT& vp, 
        int kernel_size);

    // input和output纹理宽高要求一致
    // kernel_size: 奇数, 3-15
    void ExponentialShadowLogGaussianBlur(
        ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* input, 
        ID3D11RenderTargetView* output, 
        const D3D11_VIEWPORT& vp, 
        int kernel_size, 
        float sigma);


    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class SkyboxToneMapEffect : public IEffect, public IEffectTransform,
    public IEffectMeshData
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
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;

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


#endif
