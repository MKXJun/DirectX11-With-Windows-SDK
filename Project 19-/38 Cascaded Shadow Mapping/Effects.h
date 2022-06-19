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

    void SetCascadeLevels(int cascadeLevels);
    void SetPCFDerivativesOffsetEnabled(bool enable);
    void SetCascadeBlendEnabled(bool enable);
    void SetCascadeIntervalSelectionEnabled(bool enable);

    void SetCascadeVisulization(bool enable);
    void SetCascadeOffsets(const DirectX::XMFLOAT4 offsets[8]);
    void SetCascadeScales(const DirectX::XMFLOAT4 scales[8]);
    void SetCascadeFrustumsEyeSpaceDepths(const float depths[8]);
    void SetCascadeBlendArea(float blendArea);

    void SetPCFKernelSize(int size);
    void SetPCFDepthOffset(float bias);
    
    void SetShadowSize(int size);
    void XM_CALLCONV SetShadowViewMatrix(DirectX::FXMMATRIX ShadowView);
    void SetShadowTextureArray(ID3D11ShaderResourceView* shadow);

    void SetLightDir(const DirectX::XMFLOAT3& dir);


    // 默认状态来绘制
    void SetRenderDefault(bool reversedZ = false);
    // 进行Pre-Z通道绘制
    void SetRenderPreZPass(bool reversedZ = false);


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

    // 默认状态来绘制
    void SetRenderDefault();

    // Alpha裁剪绘制(处理具有透明度的物体)
    void SetRenderAlphaClip(float alphaClipValue);

    // 绘制深度图到纹理
    void RenderDepthToTexture(ID3D11DeviceContext* deviceContext, 
        ID3D11ShaderResourceView* input, 
        ID3D11RenderTargetView* output, 
        const D3D11_VIEWPORT& vp);

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class SkyboxEffect : public IEffect, public IEffectTransform,
    public IEffectMeshData, public IEffectMaterial
{
public:
    SkyboxEffect();
    virtual ~SkyboxEffect() override;

    SkyboxEffect(SkyboxEffect&& moveFrom) noexcept;
    SkyboxEffect& operator=(SkyboxEffect&& moveFrom) noexcept;

    // 获取单例
    static SkyboxEffect& Get();

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

    void SetMaterial(const Material& material) override;

    //
    // IEffectMeshData
    //

    MeshDataInput GetInputData(const MeshData& meshData) override;

    // 
    // SkyboxEffect
    //

    // 默认状态来绘制
    void SetRenderDefault();

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
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


#endif
