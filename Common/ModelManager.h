//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#pragma once

#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include "WinMin.h"
#include "Geometry.h"
#include "Material.h"
#include "MeshData.h"
#include <d3d11_1.h>
#include <wrl/client.h>

struct Model
{
	std::vector<Material> materials;
	std::vector<MeshData> meshdatas;
	DirectX::BoundingBox boundingbox;
};


class ModelManager
{
public:
	ModelManager();
	~ModelManager();
	ModelManager(ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelManager(ModelManager&&) = default;
	ModelManager& operator=(ModelManager&&) = default;

	static ModelManager& Get();
	void Init(ID3D11Device* device);
	const Model* CreateFromFile(std::string_view filename);
	bool CreateFromGeometry(std::string_view name, const Geometry::MeshData& data);

	const Model* GetModel(std::string_view name) const;
	Model* GetModel(std::string_view name);
private:
	Microsoft::WRL::ComPtr<ID3D11Device> m_pDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	std::unordered_map<size_t, Model> m_Models;
};


#endif // MODELMANAGER_H
