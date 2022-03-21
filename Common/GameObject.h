//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#pragma once

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Geometry.h"
#include "Material.h"
#include "MeshData.h"
#include "Transform.h"
#include "IEffect.h"

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

	size_t GetMeshCount() const;
	size_t GetMaterialCount() const;
	const Material* GetMaterial(size_t idx) const;

	Material* GetMaterial(size_t idx);

	//
	// 获取包围盒
	//

	DirectX::BoundingBox GetLocalBoundingBox() const;
	DirectX::BoundingBox GetLocalBoundingBox(size_t idx) const;
	DirectX::BoundingBox GetBoundingBox() const;
	DirectX::BoundingBox GetBoundingBox(size_t idx) const;
	DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;
	DirectX::BoundingOrientedBox GetBoundingOrientedBox(size_t idx) const;

	//
	// 视锥体碰撞检测
	//
	void FrustumCulling(const DirectX::BoundingFrustum& frustumInWorld);

	//
	// 设置模型
	//

	void LoadModelFromFile(ID3D11Device* device, std::string_view filename);
	void LoadModelFromGeometry(ID3D11Device* device, const Geometry::MeshData& meshData);

	//
	// 绘制
	//

	// 绘制对象
	void Draw(ID3D11DeviceContext* deviceContext, IEffect* effect);
	
	//
	// 调试 
	//
	
	// 设置调试对象名
	// 若模型被重新设置，调试对象名也需要被重新设置
	void SetDebugObjectName(const std::string& name);

private:
	std::vector<std::shared_ptr<Material>> m_pMaterials;
	std::vector<std::shared_ptr<MeshData>> m_pMeshDatas;
	DirectX::BoundingBox m_BoundingBox;
	Transform m_Transform = {};
	bool m_InFrustum = true;
};




#endif
