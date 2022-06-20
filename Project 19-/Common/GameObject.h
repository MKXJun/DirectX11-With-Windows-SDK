//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#pragma once

#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "Geometry.h"
#include "Material.h"
#include "MeshData.h"
#include "Transform.h"
#include "IEffect.h"

struct Model;

class GameObject
{
public:
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;


    GameObject() = default;
    ~GameObject() = default;

    GameObject(const GameObject&) = default;
    GameObject& operator=(const GameObject&) = default;

    GameObject(GameObject&&) = default;
    GameObject& operator=(GameObject&&) = default;

    // 获取物体变换
    Transform& GetTransform();
    // 获取物体变换
    const Transform& GetTransform() const;

    //
    // 相交检测
    //
    void FrustumCulling(const DirectX::BoundingFrustum& frustumInWorld);
    void CubeCulling(const DirectX::BoundingOrientedBox& obbInWorld);
    void CubeCulling(const DirectX::BoundingBox& aabbInWorld);
    bool InFrustum() const { return m_InFrustum; }

    //
    // 模型
    //
    void SetModel(const Model* pModel);
    const Model* GetModel() const;

    DirectX::BoundingBox GetLocalBoundingBox() const;
    DirectX::BoundingBox GetLocalBoundingBox(size_t idx) const;
    DirectX::BoundingBox GetBoundingBox() const;
    DirectX::BoundingBox GetBoundingBox(size_t idx) const;
    DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;
    DirectX::BoundingOrientedBox GetBoundingOrientedBox(size_t idx) const;
    //
    // 绘制
    //

    void SetVisible(bool visible) {
        m_InFrustum = visible;
        m_SubModelInFrustum.assign(m_SubModelInFrustum.size(), true);
    }

    // 绘制对象
    void Draw(ID3D11DeviceContext* deviceContext, IEffect& effect);

protected:
    const Model* m_pModel = nullptr;
    std::vector<bool> m_SubModelInFrustum;
    Transform m_Transform = {};
    bool m_InFrustum = true;
};




#endif
