#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "BasicFX.h"
#include "Geometry.h"

// 一个尽可能小的游戏对象类
class GameObject
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	// 获取位置
	DirectX::XMFLOAT3 GetPosition() const;

	// 设置缓冲区
	void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData& meshData);
	// 设置纹理
	void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
	// 设置材质
	void SetMaterial(const Material& material);
	// 设置矩阵
	void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
	void SetWorldMatrix(DirectX::FXMMATRIX world);
	void SetTexTransformMatrix(const DirectX::XMFLOAT4X4& texTransform);
	void SetTexTransformMatrix(DirectX::FXMMATRIX texTransform);
	// 绘制
	void Draw(ComPtr<ID3D11DeviceContext> deviceContext);
private:
	DirectX::XMFLOAT4X4 mWorldMatrix;				// 世界矩阵
	DirectX::XMFLOAT4X4 mTexTransform;				// 纹理变换矩阵
	Material mMaterial;								// 物体材质
	ComPtr<ID3D11ShaderResourceView> mTexture;		// 纹理
	ComPtr<ID3D11Buffer> mVertexBuffer;				// 顶点缓冲区
	ComPtr<ID3D11Buffer> mIndexBuffer;				// 索引缓冲区
	int mIndexCount;								// 索引数目	
};


#endif
