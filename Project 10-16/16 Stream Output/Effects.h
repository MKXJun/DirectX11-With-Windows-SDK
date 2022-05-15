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
#include "LightHelper.h"
#include "RenderStates.h"

class IEffect
{
public:
    // 使用模板别名(C++11)简化类型名
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    IEffect() = default;
    virtual ~IEffect() = default;
    // 不允许拷贝，允许移动
    IEffect(const IEffect&) = delete;
    IEffect& operator=(const IEffect&) = delete;
    IEffect(IEffect&&) = default;
    IEffect& operator=(IEffect&&) = default;

    // 更新并绑定常量缓冲区
    virtual void Apply(ID3D11DeviceContext * deviceContext) = 0;
};


class BasicEffect : public IEffect
{
public:

    BasicEffect();
    virtual ~BasicEffect() override;

    BasicEffect(BasicEffect&& moveFrom) noexcept;
    BasicEffect& operator=(BasicEffect&& moveFrom) noexcept;

    // 获取单例
    static BasicEffect& Get();

    

    // 初始化所需资源
    bool InitAll(ID3D11Device * device);


    //
    // 渲染模式的变更
    //

    // 绘制三角形分形
    void SetRenderSplitedTriangle(ID3D11DeviceContext * deviceContext);
    // 绘制雪花
    void SetRenderSplitedSnow(ID3D11DeviceContext * deviceContext);
    // 绘制球体
    void SetRenderSplitedSphere(ID3D11DeviceContext * deviceContext);
    // 通过流输出阶段获取三角形分裂的下一阶分形
    void SetStreamOutputSplitedTriangle(ID3D11DeviceContext * deviceContext, ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut);
    // 通过流输出阶段获取雪花的下一阶分形
    void SetStreamOutputSplitedSnow(ID3D11DeviceContext * deviceContext, ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut);
    // 通过流输出阶段获取球的下一阶分形
    void SetStreamOutputSplitedSphere(ID3D11DeviceContext * deviceContext, ID3D11Buffer * vertexBufferIn, ID3D11Buffer * vertexBufferOut);

    // 绘制所有顶点的法向量
    void SetRenderNormal(ID3D11DeviceContext * deviceContext);


    //
    // 矩阵设置
    //

    void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W);
    void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
    void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);

    
    //
    // 光照、材质和纹理相关设置
    //

    // 各种类型灯光允许的最大数目
    static const int maxLights = 5;

    void SetDirLight(size_t pos, const DirectionalLight& dirLight);
    void SetPointLight(size_t pos, const PointLight& pointLight);
    void SetSpotLight(size_t pos, const SpotLight& spotLight);

    void SetMaterial(const Material& material);



    void SetEyePos(const DirectX::XMFLOAT3& eyePos);

    //
    // 设置球体
    //

    void SetSphereCenter(const DirectX::XMFLOAT3& center);
    void SetSphereRadius(float radius);

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext * deviceContext) override;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};










#endif
