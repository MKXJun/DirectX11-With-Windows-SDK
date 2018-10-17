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
	void ResizeBuffer(ComPtr<ID3D11Device> device, size_t count);
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
	void SetWorldMatrix(DirectX::FXMMATRIX world);

	//
	// 绘制
	//
	
	// 绘制对象
	void Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicFX& effect);
	// 绘制实例
	void DrawInstanced(ComPtr<ID3D11DeviceContext> deviceContext, BasicFX & effect, const std::vector<DirectX::XMMATRIX>& data);

private:
	Model mModel;												// 模型
	DirectX::XMFLOAT4X4 mWorldMatrix;							// 世界矩阵

	ComPtr<ID3D11Buffer> mInstancedBuffer;						// 实例缓冲区
	size_t mCapacity;											// 缓冲区容量
};




#endif