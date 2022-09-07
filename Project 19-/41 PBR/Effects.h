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

class DeferredEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
{
public:

    DeferredEffect();
    virtual ~DeferredEffect() override;

    DeferredEffect(DeferredEffect&& moveFrom) noexcept;
    DeferredEffect& operator=(DeferredEffect&& moveFrom) noexcept;

    // 获取单例
    static DeferredEffect& Get();

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

    void SetCascadeVisulization(bool enable);
    void SetCascadeOffsets(const DirectX::XMFLOAT4 offsets[4]);
    void SetCascadeScales(const DirectX::XMFLOAT4 scales[4]);
    void SetCascadeFrustumsEyeSpaceDepths(const float depths[4]);
    void SetCascadeBlendArea(float blendArea);
    void SetShadowSize(int size);
    void XM_CALLCONV SetShadowViewMatrix(DirectX::FXMMATRIX ShadowView);
    void SetPCFKernelSize(int size);
    void SetPCFDepthBias(float bias);
    void SetCameraNearFar(float nearZ, float farZ);

    // 设置方向光
    // 需要传入View空间方向
    void SetDirectionalLight(
        const DirectX::XMFLOAT3& lightDir, 
        const DirectX::XMFLOAT3& lightColor, 
        float lightIntensity);

    // 绘制几何缓冲区(顶点法线)
    void SetRenderGBuffer(bool alphaClip);
    // 绘制几何缓冲区(法线贴图)
    void SetRenderGBufferWithNormalMap(bool hasTangent, bool alphaClip);

    // 将法线G-Buffer渲染到目标纹理
    void DebugNormalGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* normalGBuffer,
        D3D11_VIEWPORT viewport);

    // 将Albedo渲染到目标纹理
    void DebugAlbedoGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* albedoGBuffer,
        D3D11_VIEWPORT viewport);

    // 将Roughness渲染到目标纹理
    void DebugRoughnessGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* roughnessGBuffer,
        D3D11_VIEWPORT viewport);

    // 将Metallic渲染到目标纹理
    void DebugMetallicGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* metallicGBuffer,
        D3D11_VIEWPORT viewport);


    // 执行分块光照裁剪
    void ComputeTiledLightCulling(ID3D11DeviceContext* deviceContext,
        ID3D11UnorderedAccessView* litBufferUAV,
        ID3D11ShaderResourceView* pointLightBufferSRV,      // 需要在观察空间
        ID3D11ShaderResourceView* shadowTextureArraySRV,
        ID3D11ShaderResourceView* GBuffers[3]);

    //
    // IEffect
    //

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
    void SetRenderDepthOnly();

    // 同时将深度绘制到深度图
    void SetRenderDefault();

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

class SkyboxToneMapEffect : public IEffect, public IEffectTransform
{
public:
    enum ToneMapping
    {
        ToneMapping_Standard,
        ToneMapping_Reinhard,
        ToneMapping_ACES_Coarse,
        ToneMapping_ACES
    };

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

    void RenderCubeSkyboxWithToneMap(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* cubeTexture,
        ID3D11ShaderResourceView* depthTexture,
        ID3D11ShaderResourceView* litTexture,
        ID3D11RenderTargetView* outputTexture,
        const D3D11_VIEWPORT& vp,
        ToneMapping tm = ToneMapping_Standard);

    //
    // IEffect
    //

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


class FXAAEffect
{
public:
    FXAAEffect();
     ~FXAAEffect();

    FXAAEffect(FXAAEffect&& moveFrom) noexcept;
    FXAAEffect& operator=(FXAAEffect&& moveFrom) noexcept;

    // 获取单例
    static FXAAEffect& Get();

    // 初始化所需资源
    bool InitAll(ID3D11Device* device);
    
    // major = 1 低质量, minor = 0...5
    // major = 2 中质量, minor = 0...9
    // major = 3 高质量, minor = 9
    void SetQuality(int major, int minor);
    void SetQualitySubPix(float val);
    void SetQualityEdgeThreshold(float threshold);
    void SetQualityEdgeThresholdMin(float thresholdMin);
    void EnableDebug(bool enabled);

    void RenderFXAA(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif
