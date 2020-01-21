//***************************************************************************************
// GameObject.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易游戏对象
// Simple game object.
//***************************************************************************************

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Model.h"




class GameObject
{
public:
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


	GameObject();

	// 获取位置
	DirectX::XMFLOAT3 GetPosition() const;

	//
	// 获取包围盒
	//

	DirectX::BoundingBox GetLocalBoundingBox() const;
	DirectX::BoundingBox GetBoundingBox() const;
	DirectX::BoundingOrientedBox GetBoundingOrientedBox() const;

	//
	// 设置实例缓冲区
	//

	// 获取缓冲区可容纳实例的数目
	size_t GetCapacity() const;
	// 重新设置实例缓冲区可容纳实例的数目
	void ResizeBuffer(ID3D11Device * device, size_t count);
	// 获取实例缓冲区

	//
	// 设置模型
	//

	void SetModel(Model&& model);
	void SetModel(const Model& model);

	//
	// 设置矩阵
	//

	void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
	void XM_CALLCONV SetWorldMatrix(DirectX::FXMMATRIX world);

	//
	// 绘制
	//
	
	// 绘制对象
	void Draw(ID3D11DeviceContext * deviceContext, BasicEffect& effect);
	// 绘制实例
	void DrawInstanced(ID3D11DeviceContext * deviceContext, BasicEffect & effect, const std::vector<DirectX::XMMATRIX>& data);
	
	//
	// 调试 
	//
	
	// 设置调试对象名
	// 若模型被重新设置，调试对象名也需要被重新设置
	void SetDebugObjectName(const std::string& name);

private:
	Model m_Model;												// 模型
	DirectX::XMFLOAT4X4 m_WorldMatrix;							// 世界矩阵

	ComPtr<ID3D11Buffer> m_pInstancedBuffer;						// 实例缓冲区
	size_t m_Capacity;											// 缓冲区容量
};




#endif
