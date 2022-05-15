//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Model.h"
#include "Transform.h"

class GameObject
{
public:
    // 使用模板别名(C++11)简化类型名
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
    // 获取包围盒
    //

    DirectX::BoundingBox GetLocalBoundingBox() const;
    DirectX::BoundingBox GetBoundingBox() const;
    DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;

    //
    // 设置模型
    //
    
    void SetModel(Model&& model);
    void SetModel(const Model& model);

    //
    // 绘制
    //

    // 绘制对象
    void Draw(ID3D11DeviceContext * deviceContext, BasicEffect& effect);
    
    //
    // 调试 
    //
    
    // 设置调试对象名
    // 若模型被重新设置，调试对象名也需要被重新设置
    void SetDebugObjectName(const std::string& name);

private:
    Model m_Model = {};												// 模型
    Transform m_Transform = {};										// 物体变换
};


#endif


