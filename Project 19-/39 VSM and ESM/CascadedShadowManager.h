//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 级联阴影管理
// Cascaded Shadow Manager.
//***************************************************************************************
#pragma once

#ifndef CASCADED_SHADOW_MANAGER_H
#define CASCADED_SHADOW_MANAGER_H

#include "WinMin.h"
#include <d3d11_1.h>
#include <DirectXCollision.h>
#include <wrl/client.h>
#include <memory>
#include <CameraController.h>
#include <Texture2D.h>

enum class ShadowType
{
    ShadowType_CSM,
    ShadowType_VSM,
    ShadowType_ESM,
    ShadowType_EVSM2,
    ShadowType_EVSM4
};

enum class CascadeSelection
{
    CascadeSelection_Map,
    CascadeSelection_Interval
};

enum class CameraSelection
{
    CameraSelection_Eye,
    CameraSelection_Light,
    CameraSelection_Cascade1,
    CameraSelection_Cascade2,
    CameraSelection_Cascade3,
    CameraSelection_Cascade4,
    CameraSelection_Cascade5,
    CameraSelection_Cascade6,
    CameraSelection_Cascade7,
    CameraSelection_Cascade8,
};

enum class FitNearFar
{
    FitNearFar_ZeroOne,
    FitNearFar_CascadeAABB,
    FitNearFar_SceneAABB,
    FitNearFar_SceneAABB_Intersection
};

enum class FitProjection
{
    FitProjection_ToCascade,
    FitProjection_ToScene
};

class CascadedShadowManager
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    CascadedShadowManager() = default;
    ~CascadedShadowManager() = default;
    // 不允许拷贝，允许移动
    CascadedShadowManager(const CascadedShadowManager&) = delete;
    CascadedShadowManager& operator=(const CascadedShadowManager&) = delete;
    CascadedShadowManager(CascadedShadowManager&&) = default;
    CascadedShadowManager& operator=(CascadedShadowManager&&) = default;

    HRESULT InitResource(ID3D11Device* device);

    void UpdateFrame(const Camera& viewerCamera,
        const Camera& lightCamera,
        const DirectX::BoundingBox& sceneBouindingBox);

    ID3D11RenderTargetView* GetCascadeRenderTargetView(size_t cascadeIndex) const { return m_pCSMTextureArray->GetRenderTarget(cascadeIndex); }
    ID3D11ShaderResourceView* GetCascadesOutput() const { return m_pCSMTextureArray->GetShaderResource(); }
    ID3D11ShaderResourceView* GetCascadeOutput(size_t cascadeIndex) const { return m_pCSMTextureArray->GetShaderResource(cascadeIndex); }

    ID3D11DepthStencilView* GetDepthBufferDSV() const { return m_pCSMDepthBuffer->GetDepthStencil(); }
    ID3D11ShaderResourceView* GetDepthBufferSRV() const { return m_pCSMDepthBuffer->GetShaderResource(); }

    ID3D11RenderTargetView* GetTempTextureRTV() const { return m_pCSMTempTexture->GetRenderTarget(); }
    ID3D11ShaderResourceView* GetTempTextureOutput() const { return m_pCSMTempTexture->GetShaderResource(); }
    
    const float* GetCascadePartitions() const { return m_CascadePartitionsFrustum; }
    void GetCascadePartitions(float output[8]) const { memcpy_s(output, sizeof m_CascadePartitionsFrustum, m_CascadePartitionsFrustum, sizeof m_CascadePartitionsFrustum); }
    DirectX::XMMATRIX GetShadowProjectionXM(size_t cascadeIndex) const { return XMLoadFloat4x4(&m_ShadowProj[cascadeIndex]); }
    DirectX::BoundingBox GetShadowAABB(size_t cascadeIndex) const { return m_ShadowProjBoundingBox[cascadeIndex]; }
    DirectX::BoundingOrientedBox GetShadowOBB(size_t cascadeIndex) const {
        DirectX::BoundingOrientedBox obb;
        DirectX::BoundingOrientedBox::CreateFromBoundingBox(obb, GetShadowAABB(cascadeIndex));
        return obb; 
    }
    const D3D11_VIEWPORT& GetShadowViewport() const { return m_ShadowViewport; }

public:
    //
    // 级联创建相关的配置
    //

    ShadowType  m_ShadowType    = ShadowType::ShadowType_VSM;
    int         m_ShadowSize    = 1024;
    int         m_ShadowBits    = 4;
    int         m_CascadeLevels = 4;
    bool        m_GenerateMips = false;

    //
    // 级联每帧相关的配置
    //
    float		m_CascadePartitionsPercentage[8]{       // 0到100的值表示视锥体所占百分比
        0.04f, 0.10f, 0.25f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
    };
    int		    m_BlurKernelSize = 5;                   // 卷积核大小(奇数)
    float       m_GaussianBlurSigma = 3.0f;             // 高斯模糊sigma(ESM)
    float       m_BlendBetweenCascadesRange = 0.2f;     // 级联混合地带的范围
    float       m_PCFDepthBias = 0.001f;                // 阴影偏移(PCF)
    float       m_LightBleedingReduction = 0.8f;        // 处理漏光用的参数(VSM)
    float       m_MagicPower = 160.0f;                  // 处理漏光用的指数(ESM)
    float       m_PosExp = 5.0f;                        // 处理漏光用的正项指数(EVSM)
    float       m_NegExp = 5.0f;                        // 处理漏光用的负项指数(EVSM)

    bool        m_FixedSizeFrustumAABB = true;          // 是否固定视锥体产生的AABB宽高
    bool        m_MoveLightTexelSize = true;            // 是否进行逐光照texel移动

    CameraSelection     m_SelectedCamera = CameraSelection::CameraSelection_Eye;
    FitProjection       m_SelectedCascadesFit = FitProjection::FitProjection_ToCascade;
    FitNearFar          m_SelectedNearFarFit = FitNearFar::FitNearFar_SceneAABB_Intersection;
    CascadeSelection    m_SelectedCascadeSelection = CascadeSelection::CascadeSelection_Map;
    
private:
    void XM_CALLCONV ComputeNearAndFar(float& outNearPlane, float& outFarPlane,
        DirectX::FXMVECTOR lightCameraOrthographicMinVec,
        DirectX::FXMVECTOR lightCameraOrthographicMaxVec,
        DirectX::XMVECTOR pointsInCameraView[]);

private:

    float	                        m_CascadePartitionsFrustum[8]{};    // 级联远平面Z值
    DirectX::XMFLOAT4X4             m_ShadowProj[8]{};                  // 阴影正交矩阵
    DirectX::BoundingBox            m_ShadowProjBoundingBox[8]{};       // 正交矩阵对应的默认AABB
    D3D11_VIEWPORT                  m_ShadowViewport{};                 // 阴影图视口

    std::unique_ptr<Texture2DArray> m_pCSMTextureArray;
    std::unique_ptr<Texture2D> m_pCSMTempTexture;
    std::unique_ptr<Depth2D>   m_pCSMDepthBuffer;
};

#endif
