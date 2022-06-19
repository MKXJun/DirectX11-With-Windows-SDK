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

    void SetMsaaSamples(UINT msaaSamples);

    void SetLightBuffer(ID3D11ShaderResourceView* lightBuffer);
    void SetTileBuffer(ID3D11ShaderResourceView* tileBuffer);
    void SetCameraNearFar(float nearZ, float farZ);

    void SetLightingOnly(bool enable);
    void SetFaceNormals(bool enable);
    void SetVisualizeLightCount(bool enable);

    // 默认状态来绘制
    void SetRenderDefault();

    // 进行Pre-Z通道绘制
    void SetRenderPreZPass();
    // 执行分块光照裁剪
    void ComputeTiledLightCulling(ID3D11DeviceContext* deviceContext,
        ID3D11UnorderedAccessView* tileInfoBufferUAV,
        ID3D11ShaderResourceView* lightBufferSRV,
        ID3D11ShaderResourceView* depthBufferSRV);
    // 根据裁剪后的光照数据绘制
    void SetRenderWithTiledLightCulling();

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
    // 设置TBDR场景渲染图
    void SetFlatLitTexture(ID3D11ShaderResourceView* flatLitTexture, UINT width, UINT height);

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

    // 
    // BasicDeferredEffect
    //

    void SetMsaaSamples(UINT msaaSamples);

    void SetLightingOnly(bool enable);
    void SetFaceNormals(bool enable);
    void SetVisualizeLightCount(bool enable);
    void SetVisualizeShadingFreq(bool enable);

    void SetCameraNearFar(float nearZ, float farZ);

    // 绘制G缓冲区
    void SetRenderGBuffer();

    // 将法线G-Buffer渲染到目标纹理
    void DebugNormalGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* normalGBuffer,
        D3D11_VIEWPORT viewport);

    // 将深度值梯度的G-Buffer渲染到到目标纹理
    void DebugPosZGradGBuffer(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* rtv,
        ID3D11ShaderResourceView* posZGradGBuffer,
        D3D11_VIEWPORT viewport);

    // 传统延迟渲染
    void ComputeLightingDefault(ID3D11DeviceContext* deviceContext,
        ID3D11RenderTargetView* litBufferRTV,
        ID3D11DepthStencilView* depthBufferReadOnlyDSV,
        ID3D11ShaderResourceView* lightBufferSRV,
        ID3D11ShaderResourceView* GBuffers[4],
        D3D11_VIEWPORT viewport);

    // 执行分块光照裁剪
    void ComputeTiledLightCulling(ID3D11DeviceContext* deviceContext,
        ID3D11UnorderedAccessView* litFlatBufferUAV,
        ID3D11ShaderResourceView* lightBufferSRV,
        ID3D11ShaderResourceView* GBuffers[4]);

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
