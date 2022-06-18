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
    // 带法线贴图的绘制
    void SetRenderWithNormalMap();

    // 天空盒
    void SetTextureCube(ID3D11ShaderResourceView* textureCube);

    // 阴影
    void XM_CALLCONV SetShadowTransformMatrix(DirectX::FXMMATRIX S);
    void SetDepthBias(float bias);
    void SetTextureShadowMap(ID3D11ShaderResourceView* textureShadowMap);

    // 各种类型灯光允许的最大数目
    static const int maxLights = 5;

    void SetDirLight(uint32_t pos, const DirectionalLight& dirLight);
    void SetPointLight(uint32_t pos, const PointLight& pointLight);
    void SetSpotLight(uint32_t pos, const SpotLight& spotLight);

    void SetEyePos(const DirectX::XMFLOAT3& eyePos);



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
    // Alpha裁剪绘制(处理具有透明度的物体)
    void SetRenderAlphaClip();

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
    public IEffectMaterial, public IEffectMeshData
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
