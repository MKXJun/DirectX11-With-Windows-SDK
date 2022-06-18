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
    void SetShadowEnabled(bool enabled);
    void SetDepthBias(float bias);
    void SetTextureShadowMap(ID3D11ShaderResourceView* textureShadowMap);

    // SSAO
    void SetSSAOEnabled(bool enabled);
    void SetTextureAmbientOcclusion(ID3D11ShaderResourceView* textureAmbientOcclusion);

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

class SSAOEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
{
public:

    SSAOEffect();
    virtual ~SSAOEffect() override;

    SSAOEffect(SSAOEffect&& moveFrom) noexcept;
    SSAOEffect& operator=(SSAOEffect&& moveFrom) noexcept;

    // 获取单例
    static SSAOEffect& Get();

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
    // SSAOEffect
    //

    // Pass1: 绘制观察空间法向量和深度贴图
    void SetRenderNormalDepthMap(bool enableAlphaClip = false);

    // Pass2: 绘制SSAO图
     
    // 设置随机向量纹理
    void SetTextureRandomVec(ID3D11ShaderResourceView* textureRandomVec);
    // 设置遮蔽信息
    void SetOcclusionInfo(float radius, float fadeStart, float fadeEnd, float surfaceEpsilon);
    // 设置视锥体远平面信息
    void SetFrustumFarPlanePoints(const DirectX::XMFLOAT4 farPlanePoints[3]);
    // 设置偏移向量
    void SetOffsetVectors(const DirectX::XMFLOAT4 offsetVectors[14]);

    void RenderToSSAOTexture(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* normalDepth,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp,
        uint32_t sampleCount);


    // Pass3: 对SSAO图进行双边滤波

    // 设置模糊权值
    void SetBlurWeights(const float weights[11]);
    // 设置模糊半径
    void SetBlurRadius(int radius);

    // 进行水平双边滤波，要求输入输出图像大小相同
    void BilateralBlurX(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11ShaderResourceView* normalDepth,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

    // 进行竖直双边滤波，要求输入输出图像大小相同
    void BilateralBlurY(
        ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11ShaderResourceView* normalDepth,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);


    // 绘制AO图到纹理
    void RenderAmbientOcclusionToTexture(ID3D11DeviceContext* deviceContext,
        ID3D11ShaderResourceView* input,
        ID3D11RenderTargetView* output,
        const D3D11_VIEWPORT& vp);

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
