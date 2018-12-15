#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Effects.h"
#include "Geometry.h"

// 一个尽可能小的游戏对象类
class GameObject
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GameObject();

	// 获取位置
	DirectX::XMFLOAT3 GetPosition() const;

	// 设置缓冲区
	template<class VertexType, class IndexType>
	void SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);
	// 设置纹理
	void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
	// 设置材质
	void SetMaterial(const Material& material);
	// 设置矩阵
	void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
	void SetWorldMatrix(DirectX::FXMMATRIX world);

	// 绘制
	void Draw(ComPtr<ID3D11DeviceContext> deviceContext, BasicEffect& effect);
private:
	DirectX::XMFLOAT4X4 mWorldMatrix;				// 世界矩阵
	Material mMaterial;								// 物体材质
	ComPtr<ID3D11ShaderResourceView> mTexture;		// 纹理
	ComPtr<ID3D11Buffer> mVertexBuffer;				// 顶点缓冲区
	ComPtr<ID3D11Buffer> mIndexBuffer;				// 索引缓冲区
	UINT mVertexStride;								// 顶点字节大小
	UINT mIndexCount;								// 索引数目	
};

template<class VertexType, class IndexType>
inline void GameObject::SetBuffer(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	// 释放旧资源
	mVertexBuffer.Reset();
	mIndexBuffer.Reset();

	// 检查D3D设备
	if (device == nullptr)
		return;

	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexType);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	device->CreateBuffer(&vbd, &InitData, mVertexBuffer.GetAddressOf());

	// 设置索引缓冲区描述
	mIndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = mIndexCount * sizeof(IndexType);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	device->CreateBuffer(&ibd, &InitData, mIndexBuffer.GetAddressOf());

	
}


#endif
