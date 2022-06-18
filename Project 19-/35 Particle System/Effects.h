//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易特效管理框架
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include <memory>
#include <LightHelper.h>
#include <RenderStates.h>

#include <GameObject.h>

#include <Buffer.h>
#include <IEffect.h>
#include <Material.h>
#include <MeshData.h>
#include <LightHelper.h>

class BasicEffect : public IEffect, public IEffectTransform,
    public IEffectMaterial, public IEffectMeshData
{
public:
    struct InstancedData
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 worldInvTranspose;
    };

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

    // 绘制实例
    void DrawInstanced(ID3D11DeviceContext* deviceContext, Buffer& instancedBuffer, const GameObject& object, uint32_t numObjects);

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

class ParticleEffect : public IEffect
{
public:
    struct VertexParticle
    {
        DirectX::XMFLOAT3 initialPos;
        DirectX::XMFLOAT3 initialVel;
        DirectX::XMFLOAT2 size;
        float age;
        uint32_t type;
    };

    struct InputData
    {
        ID3D11InputLayout* pInputLayout;
        D3D11_PRIMITIVE_TOPOLOGY topology;
        uint32_t stride;
        uint32_t offset;
    };

public:
    ParticleEffect();
    virtual ~ParticleEffect() override;

    ParticleEffect(ParticleEffect&& moveFrom) noexcept;
    ParticleEffect& operator=(ParticleEffect&& moveFrom) noexcept;

    bool InitAll(ID3D11Device* device, std::wstring_view filename);

    // vertexCount为0时调用drawAuto
    void RenderToVertexBuffer(
        ID3D11DeviceContext* deviceContext,
        ID3D11Buffer* input,
        ID3D11Buffer* output,
        uint32_t vertexCount = 0);  
    // 绘制粒子系统
    InputData SetRenderDefault();

    void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
    void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);

    void SetEyePos(const DirectX::XMFLOAT3& eyePos);

    void SetGameTime(float t);
    void SetTimeStep(float step);

    void SetEmitDir(const DirectX::XMFLOAT3& dir);
    void SetEmitPos(const DirectX::XMFLOAT3& pos);

    void SetEmitInterval(float t);
    void SetAliveTime(float t);

    void SetAcceleration(const DirectX::XMFLOAT3& accel);

    void SetTextureInput(ID3D11ShaderResourceView* textureInput);
    void SetTextureRandom(ID3D11ShaderResourceView* textureRandom);

    void SetRasterizerState(ID3D11RasterizerState* rasterizerState);
    void SetBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask);
    void SetDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef);

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
