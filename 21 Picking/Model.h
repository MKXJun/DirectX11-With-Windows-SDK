//***************************************************************************************
// Model.h by X_Jun(MKXJun) (C) 2018-2019 All Rights Reserved.
// Licensed under the MIT License.
//
// 存放模型数据
// model data storage.
//***************************************************************************************

#ifndef MODEL_H
#define MODEL_H

#include <DirectXCollision.h>
#include "Effects.h"
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

struct Model
{
	// 使用模板别名(C++11)简化类型名
	template <class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
	
	Model();
	Model(ComPtr<ID3D11Device> device, const ObjReader& model);
	// 设置缓冲区
	template<class VertexType, class IndexType>
	Model(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);
	
	template<class VertexType, class IndexType>
	Model(ComPtr<ID3D11Device> device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices);
	
	
	Model(ComPtr<ID3D11Device> device, const void* vertices, UINT vertexSize, UINT vertexCount,
		const void * indices, UINT indexCount, DXGI_FORMAT indexFormat);
	//
	// 设置模型
	//

	void SetModel(ComPtr<ID3D11Device> device, const ObjReader& model);

	//
	// 设置网格
	//
	template<class VertexType, class IndexType>
	void SetMesh(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData);

	template<class VertexType, class IndexType>
	void SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices);

	void SetMesh(ComPtr<ID3D11Device> device, const void* vertices, UINT vertexSize, UINT vertexCount,
		const void * indices, UINT indexCount, DXGI_FORMAT indexFormat);

	std::vector<ModelPart> modelParts;
	DirectX::BoundingBox boundingBox;
	UINT vertexStride;
};




template<class VertexType, class IndexType>
inline Model::Model(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	SetMesh(device, meshData);
}

template<class VertexType, class IndexType>
inline Model::Model(ComPtr<ID3D11Device> device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices)
{
	SetMesh(device, vertices, indices);
}

template<class VertexType, class IndexType>
inline void Model::SetMesh(ComPtr<ID3D11Device> device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	SetMesh(device, meshData.vertexVec, meshData.indexVec);
}

template<class VertexType, class IndexType>
inline void Model::SetMesh(ComPtr<ID3D11Device> device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices)
{
	static_assert(sizeof(IndexType) == 2 || sizeof(IndexType) == 4, "The size of IndexType must be 2 bytes or 4 bytes!");
	static_assert(std::is_unsigned<IndexType>::value, "IndexType must be unsigned integer!");
	SetMesh(device, vertices.data(), sizeof(VertexType),
		(UINT)vertices.size(), indices.data(), (UINT)indices.size(),
		(sizeof(IndexType) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT));

}



#endif
