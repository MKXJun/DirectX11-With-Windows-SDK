#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <DirectXCollision.h>
#include "BasicFX.h"
#include "ObjReader.h"
#include "Geometry.h"

struct ModelPart
{
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	Material material;
	ComPtr<ID3D11ShaderResourceView> texA;
	ComPtr<ID3D11ShaderResourceView> texD;
	ComPtr<ID3D11Buffer> vertexBuffer;
	ComPtr<ID3D11Buffer> indexBuffer;
	UINT vertexCount;
	UINT indexCount;
	DXGI_FORMAT indexFormat;
};


class GameObject
{
public:
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	GameObject();

	// 获取位置
	DirectX::XMFLOAT3 GetPosition() const;
	// 获取子模型
	const ModelPart& GetModelPart(size_t pos) const;
	size_t GetModelPartSize() const;
	// 获取包围盒
	void GetBoundingBox(DirectX::BoundingBox& box) const;
	void GetBoundingBox(DirectX::BoundingBox& box, DirectX::FXMMATRIX worldMatrix) const;

	//
	// 设置模型
	//
	
	void SetModel(ComPtr<ID3D11Device> device, const ObjReader& model);
	

	//
	// 设置网格
	//

	void SetMesh(ComPtr<ID3D11Device> device, const Geometry::MeshData& meshData);
	void SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<WORD> indices);
	void SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexPosNormalTex>& vertices, const std::vector<DWORD> indices);
	
	//
	// 设置纹理
	//

	void SetTexture(ComPtr<ID3D11ShaderResourceView> texture);
	void SetTexture(ComPtr<ID3D11ShaderResourceView> texA, ComPtr<ID3D11ShaderResourceView> texD);
	void SetTexture(size_t partIndex, ComPtr<ID3D11ShaderResourceView> texture);
	void SetTexture(size_t partIndex, ComPtr<ID3D11ShaderResourceView> texA, ComPtr<ID3D11ShaderResourceView> texD);
	
	//
	// 设置材质
	//

	void SetMaterial(const Material& material);
	void SetMaterial(size_t partIndex, const Material& material);
	
	//
	// 设置矩阵
	//

	void SetWorldMatrix(const DirectX::XMFLOAT4X4& world);
	void SetWorldMatrix(DirectX::FXMMATRIX world);
	void SetTexTransformMatrix(const DirectX::XMFLOAT4X4& texTransform);
	void SetTexTransformMatrix(DirectX::FXMMATRIX texTransform);


	// 绘制对象
	void Draw(ComPtr<ID3D11DeviceContext> deviceContext);

private:
	void SetMesh(ComPtr<ID3D11Device> device, const VertexPosNormalTex* vertices, UINT vertexCount,
		const void * indices, UINT indexCount, DXGI_FORMAT indexFormat);

private:
	DirectX::XMFLOAT4X4 mWorldMatrix;							// 世界矩阵
	DirectX::XMFLOAT4X4 mTexTransform;							// 纹理变换矩阵
	std::vector<ModelPart> mParts;								// 模型的各个部分
	DirectX::BoundingBox mBoundingBox;							// 模型的AABB盒
};


#endif


