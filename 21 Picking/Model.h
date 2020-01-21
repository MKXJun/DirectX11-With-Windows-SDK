//***************************************************************************************
// Model.h by X_Jun(MKXJun) (C) 2018-2020 All Rights Reserved.
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

	ModelPart() : material(), texDiffuse(), vertexBuffer(), indexBuffer(),
		vertexCount(), indexCount(), indexFormat() {}

	ModelPart(const ModelPart&) = default;
	ModelPart& operator=(const ModelPart&) = default;

	ModelPart(ModelPart&&) = default;
	ModelPart& operator=(ModelPart&&) = default;


	Material material;
	ComPtr<ID3D11ShaderResourceView> texDiffuse;
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
	Model(ID3D11Device * device, const ObjReader& model);
	// 设置缓冲区
	template<class VertexType, class IndexType>
	Model(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData);
	
	template<class VertexType, class IndexType>
	Model(ID3D11Device * device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices);
	
	
	Model(ID3D11Device * device, const void* vertices, UINT vertexSize, UINT vertexCount,
		const void * indices, UINT indexCount, DXGI_FORMAT indexFormat);
	//
	// 设置模型
	//

	void SetModel(ID3D11Device * device, const ObjReader& model);

	//
	// 设置网格
	//
	template<class VertexType, class IndexType>
	void SetMesh(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData);

	template<class VertexType, class IndexType>
	void SetMesh(ID3D11Device * device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices);

	void SetMesh(ID3D11Device * device, const void* vertices, UINT vertexSize, UINT vertexCount,
		const void * indices, UINT indexCount, DXGI_FORMAT indexFormat);

	//
	// 调试 
	//

	// 设置调试对象名
	// 若模型被重新设置，调试对象名也需要被重新设置
	void SetDebugObjectName(const std::string& name);

	std::vector<ModelPart> modelParts;
	DirectX::BoundingBox boundingBox;
	UINT vertexStride;
};




template<class VertexType, class IndexType>
inline Model::Model(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData)
	: modelParts(), boundingBox(), vertexStride()
{
	SetMesh(device, meshData);
}

template<class VertexType, class IndexType>
inline Model::Model(ID3D11Device * device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices)
	: modelParts(), boundingBox(), vertexStride()
{
	SetMesh(device, vertices, indices);
}

template<class VertexType, class IndexType>
inline void Model::SetMesh(ID3D11Device * device, const Geometry::MeshData<VertexType, IndexType>& meshData)
{
	SetMesh(device, meshData.vertexVec, meshData.indexVec);
}

template<class VertexType, class IndexType>
inline void Model::SetMesh(ID3D11Device * device, const std::vector<VertexType> & vertices, const std::vector<IndexType>& indices)
{
	static_assert(sizeof(IndexType) == 2 || sizeof(IndexType) == 4, "The size of IndexType must be 2 bytes or 4 bytes!");
	static_assert(std::is_unsigned<IndexType>::value, "IndexType must be unsigned integer!");
	SetMesh(device, vertices.data(), sizeof(VertexType),
		(UINT)vertices.size(), indices.data(), (UINT)indices.size(),
		(sizeof(IndexType) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT));

}



#endif
