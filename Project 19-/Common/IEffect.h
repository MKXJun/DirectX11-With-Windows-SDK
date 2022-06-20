//***************************************************************************************
// IEffects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 特效基类定义
// Effect interface definitions.
//***************************************************************************************

#ifndef IEFFECT_H
#define IEFFECT_H

#include "WinMin.h"
#include <memory>
#include <vector>
#include <d3d11_1.h>
#include <DirectXMath.h>

class Material;
struct MeshData;

// 单个MeshData需要设置到输入装配阶段的内容
// 输入布局、strides、offsets和图元由Effect Pass提供
// 其余数据源自MeshData
struct MeshDataInput
{
    ID3D11InputLayout* pInputLayout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    std::vector<ID3D11Buffer*> pVertexBuffers;
    ID3D11Buffer* pIndexBuffer = nullptr;
    std::vector<uint32_t> strides;
    std::vector<uint32_t> offsets;
    uint32_t indexCount = 0;
};

class IEffect
{
public:
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

class IEffectTransform
{
public:
    virtual void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX W) = 0;
    virtual void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V) = 0;
    virtual void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P) = 0;
};

class IEffectMaterial
{
public:
    virtual void SetMaterial(const Material& material) = 0;
};

class IEffectMeshData
{
public:
    virtual MeshDataInput GetInputData(const MeshData& meshData) = 0;
};


#endif
